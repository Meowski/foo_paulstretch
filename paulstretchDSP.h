#pragma once
#include "stdafx.h"
#include "resource.h"
#include "paulstretch.h"
#include <queue>
#include <chrono>


class dsp_paulstretch : public dsp_impl_base
{
public:
	static float stretch_amount;
	static float gain_amount;
	static float window_size;
	static bool enabled;

	dsp_paulstretch() :
		myLastSeenWindowSize(window_size), myLastSeenNumberOfChannels(2), myLastSeenSampleRate(0), 
		myLastSeenChannelConfig(0), myPreviousWindows(64) {
		myLastTimeWindowSizeChanged = std::chrono::system_clock::now();
	}

	static GUID g_get_guid() {
		static const GUID guid = { 0x5efad0c, 0x7322, 0x40ce,{ 0x94, 0x8e, 0xfe, 0x82, 0xba, 0x67, 0x1, 0x72 } };
		return guid;
	}

	GUID get_owner()
	{
		return g_get_guid();
	}

	static void g_get_name(pfc::string_base & p_out) { p_out = "Paulstretch DSP"; }

	bool on_chunk(audio_chunk * chunk, abort_callback &) {

		if (!enabled)
			return true;

		// All state required to do paulstretch is saved after this call, so all 'myLastSeen...' 
		// variable usage is fresh.
		//
		remember_state(chunk);

		size_t data_length = chunk->get_used_size();
		audio_sample* the_data = chunk->get_data();
		for (size_t i = 0; i < data_length; i++)
			myQueue.push_back(the_data[i]);

		size_t window_size_in_samples = get_window_size_in_samples(myLastSeenWindowSize, myLastSeenSampleRate);
		if (myQueue.size() >= sampleCountRequiredForStretch(window_size_in_samples, myLastSeenNumberOfChannels))
			handle_stretch(myLastSeenWindowSize, stretch_amount, myLastSeenNumberOfChannels, myLastSeenSampleRate, myLastSeenChannelConfig);

		// We need to buffer chunks on our own, so drop everything.
		//
		return false;
	}

	void remember_state(audio_chunk* chunk)
	{
		// We only want to change our window size periodically. Otherwise we really hammer memory with
		// allocations.
		//
		if (myLastSeenWindowSize != window_size)
		{
			std::chrono::system_clock::time_point current_time_point = std::chrono::system_clock::now();
			auto duration = current_time_point - myLastTimeWindowSizeChanged;
			if (duration > std::chrono::seconds(1)) {
				myLastSeenWindowSize = window_size;
				myLastTimeWindowSizeChanged = current_time_point;
			}
		}

		myLastSeenNumberOfChannels = chunk->get_channels();
		myLastSeenSampleRate = chunk->get_sample_rate();
		myLastSeenChannelConfig = chunk->get_channel_config();
	}

	void handle_stretch(float window_size_in_seconds, float stretch_rate, size_t n_channels, unsigned int s_rate, unsigned int channel_config)
	{
		// Window size sample per channel.
		//
		std::unique_ptr<sample_vector<audio_sample>[]> samples(new sample_vector<audio_sample>[n_channels]);
		size_t window_size_samples = get_window_size_in_samples(window_size_in_seconds, s_rate);
		while (myQueue.size() >= sampleCountRequiredForStretch(window_size_samples, n_channels))
		{
			for (size_t i = 0; i < n_channels; i++)
				samples[i] = sample_vector<audio_sample>(window_size_samples);

			// channels are identified in the sample modulo 'n_channels'
			//
			for (size_t i = 0; i < window_size_samples; i++) 
				for (size_t channel = 0; channel < n_channels; channel++) 
					samples[channel][i] = myQueue[n_channels * i + channel];

			// stretch each sample
			//
			std::vector<sample_vector<audio_sample>> results;
			for (size_t i = 0; i < n_channels; i++) {
				samples[i] = paulstretch::stateless_stretch(stretch_rate, samples[i]);
				results.push_back(paulstretch::combine_windows(window_size_samples, myPreviousWindows[i], samples[i]));
				myPreviousWindows[i] = samples[i];
			}


			// insert it into the chunk stream
			//
			audio_chunk* new_chunk = insert_chunk();
			std::unique_ptr<audio_sample[]> new_audio_sample(new audio_sample[results[0].get_size() * n_channels]);
			for (size_t i = 0; i < results[0].get_size(); i++) {
				for (size_t j = 0; j < n_channels; j++)
					new_audio_sample[i * n_channels + j] = results[j][i];
			}

			// does a deep copy of audio samples, so we retain ownership of pointer.
			//
			new_chunk->set_data(new_audio_sample.get(), results[0].get_size(),
				n_channels, s_rate, channel_config);

			// now remove stepsize from circular buffer (half the window size typically)
			//
			// Note, to avoid infinite loops, it must be that we take at least one step in the sample:
			//
			//		(window_size_in_samples / 2.0f) / stretch_rate >= 1
			//
			// meaning:
			//
			//		window_size_in_samples >= 2 * stretch_rate
			//
			// When this does not happen, we will enforce a step of one.
			//
			size_t step = static_cast<size_t>(paulstretch::step_size(window_size_samples, stretch_rate) * n_channels);
			step = max(1, step);
			for (size_t i = 0; i < step; i++) {
				if (myQueue.empty())
					break; // shouldn't happen, but we'll check anyhow.
				myQueue.pop_front();
			}
		}
	}

	static size_t get_window_size_in_samples(float window_size_in_seconds, size_t sample_rate)
	{
		return paulstretch::required_sample_size(window_size_in_seconds, sample_rate);
	}

	static size_t sampleCountRequiredForStretch(size_t window_size_samples, size_t n_channels)
	{
		return window_size_samples * n_channels;
	}

	void on_endofplayback(abort_callback &) {}
	void on_endoftrack(abort_callback &) {

		if (!enabled)
			return;

		// For paulstretch, we need to pad with 0s for the last window to process.  We pad just enough
		// to trigger one more stretch chunk to be inserted.
		//
		size_t window_size_in_samples = get_window_size_in_samples(myLastSeenWindowSize, myLastSeenSampleRate);
		while (myQueue.size() < sampleCountRequiredForStretch(window_size_in_samples, myLastSeenNumberOfChannels))
			myQueue.push_back(0);
		handle_stretch(myLastSeenWindowSize, stretch_amount, myLastSeenNumberOfChannels, myLastSeenSampleRate, myLastSeenChannelConfig);
	}

	// If you have any audio data buffered, you should drop it immediately and reset the DSP to a freshly initialized state.
	// Called after a seek etc.
	//
	void flush() {
		// We could flush, but I don't think it has a bad effect on the playback, so why mess with any allocated memory.
	}

	double get_latency() {
		// If the DSP buffers some amount of audio data, it should return the duration of buffered data (in seconds) here.
		return 0;
	}

	// Return true if you need on_endoftrack() or need to accurately know which track we're currently processing
	// WARNING: If you return true, the DSP manager will fire on_endofplayback() at DSPs that are before us in the chain on track change to ensure that we get an accurate mark, so use it only when needed.
	//
	bool need_track_change_mark() {
		// We need to buffer one extra window for paulstretch.
		//
		return true;
	}

private:

	float myLastSeenWindowSize;
	size_t myLastSeenNumberOfChannels;
	size_t myLastSeenSampleRate;
	size_t myLastSeenChannelConfig;

	std::chrono::system_clock::time_point myLastTimeWindowSizeChanged;
	std::vector<sample_vector<audio_sample>> myPreviousWindows;
	std::deque<audio_sample> myQueue;
};

float dsp_paulstretch::stretch_amount = 4.0f;
float dsp_paulstretch::window_size = 0.28f;
bool dsp_paulstretch::enabled = false;

// Use dsp_factory_nopreset_t<> instead of dsp_factory_t<> if your DSP does not provide preset/configuration functionality.
static dsp_factory_nopreset_t<dsp_paulstretch> g_dsp_sample_factory;

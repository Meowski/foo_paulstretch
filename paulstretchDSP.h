#pragma once
#include "stdafx.h"
#include "resource.h"
#include "paulstretch.h"
#include <queue>
#include <chrono>
#include <iostream>


class dsp_paulstretch : public dsp_impl_base
{
public:
	static float stretch_amount;
	static float gain_amount;
	static float window_size;
	static bool enabled;

	dsp_paulstretch() :
		myLastSeenNumberOfChannels(-1), 
		myLastSeenSampleRate(-1), 
		myLastSeenChannelConfig(-1)
	{
		myLastTimeWindowSizeChanged = std::chrono::system_clock::now() - std::chrono::hours(1);
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
		splitAndFeed(chunk);
		while(canStretch())
			stretch(stretch_amount);
		
		// We need to buffer chunks on our own, so drop everything.
		//
		return false;
	}

	bool canStretch()
	{
		return !myPaulstretch.empty() && canAllStep();
	}

	void stretch(const float stretch_amount)
	{
		std::vector<AudioBuffer*> output(myLastSeenNumberOfChannels);
		for (size_t i = 0; i < myLastSeenNumberOfChannels; i++) 
			output[i] = myPaulstretch[i].step(stretch_amount);
		if (!output.empty() && !output[0]->empty())
			combineAndOutput(output);
	}

	bool canAllStep()
	{
		if (myPaulstretch.size() == 0)
			return false;

		bool result = true;
		for (size_t i = 0; i < myPaulstretch.size() && i < myLastSeenNumberOfChannels; i++)
			result = result && myPaulstretch[i].canStep();
		return result;
	}

	void combineAndOutput(const std::vector<AudioBuffer*>& results)
	{
		audio_chunk* new_chunk = insert_chunk();
		size_t result_size = results[0]->size() * results.size();
		std::unique_ptr<audio_sample[]> new_audio_sample(new audio_sample[result_size]);
		for (size_t i = 0; i < results[0]->size(); i++) {
			for (size_t j = 0; j < myLastSeenNumberOfChannels; j++)
				new_audio_sample[i * myLastSeenNumberOfChannels + j] = results[j]->get(i);
		}

		// does a deep copy of audio samples, so we retain ownership of pointer.
		//
		new_chunk->set_data(new_audio_sample.get(), results[0]->size(),
			myLastSeenNumberOfChannels, myLastSeenSampleRate, myLastSeenChannelConfig);
	}

	void splitAndFeed(audio_chunk* chunk)
	{
		size_t data_length = chunk->get_used_size();
		audio_sample* the_data = chunk->get_data();
		for (size_t i = 0; i < data_length / myLastSeenNumberOfChannels; i++)
		{
			for (size_t j = 0; j < myLastSeenNumberOfChannels; j++)
			{
				size_t index = i * myLastSeenNumberOfChannels + j;
				if (index >= data_length)
					break;

				myPaulstretch[j].feed(the_data[index]);
			}
		}
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
				resizePaulstretch(chunk, chunk->get_channels(), window_size);
			}
		}
		if (myLastSeenNumberOfChannels != chunk->get_channels())
		{
			resizePaulstretch(chunk, chunk->get_channels(), window_size);
		}
		else if (myLastSeenSampleRate != chunk->get_sample_rate())
		{
			resizePaulstretch(chunk, chunk->get_channels(), window_size);
		}

		myLastSeenNumberOfChannels = chunk->get_channels();
		myLastSeenSampleRate = chunk->get_sample_rate();
		myLastSeenChannelConfig = chunk->get_channel_config();
	}

	void resizePaulstretch(audio_chunk* chunk, size_t n_channels, const float window_size)
	{
		while (myPaulstretch.size() > n_channels)
			myPaulstretch.pop_back();
		while (myPaulstretch.size() < n_channels)
			myPaulstretch.push_back(NewPaulstretch(window_size, chunk->get_sample_rate()));
		for (size_t i = 0; i < myPaulstretch.size(); i++)
			myPaulstretch[i].resize(window_size, chunk->get_sample_rate());
	}

	void on_endofplayback(abort_callback &) {}
	void on_endoftrack(abort_callback &) {

		if (!enabled)
			return;

		// For paulstretch, we need to pad with 0s for the last window to process.  We pad just enough
		// to trigger one more stretch chunk to be inserted.
		//
		float stretchAmount = stretch_amount;
		for (size_t i = 0; i < myPaulstretch.size(); i++)
		{
			while (!myPaulstretch[i].canStep())
				myPaulstretch[i].feed(0.0f);
		}
		stretch(stretchAmount);
	}

	// If you have any audio data buffered, you should drop it immediately and reset the DSP to a freshly initialized state.
	// Called after a seek etc.
	//
	void flush() {
		// We could flush, but I think the gradual fade introduced by not doing so sounds nice.
	}

	double get_latency() {
		// If the DSP buffers some amount of audio data, it should return the duration of buffered data (in seconds) here.
		if(enabled)
			return myLastSeenWindowSize;	// rough estimate
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
	std::vector<NewPaulstretch> myPaulstretch;
};

float dsp_paulstretch::stretch_amount = 4.0f;
float dsp_paulstretch::window_size = 0.28f;
bool dsp_paulstretch::enabled = false;

// Use dsp_factory_nopreset_t<> instead of dsp_factory_t<> if your DSP does not provide preset/configuration functionality.
static dsp_factory_nopreset_t<dsp_paulstretch> g_dsp_sample_factory;

#pragma once
#ifndef PAULSTRETCHDSP_H
#define PAULSTRETCHDSP_H


#include "../SDK/foobar2000.h"
#include "paulstretch.h"
#include <queue>
#include <chrono>
#include "paulstretchPreset.h"
#include "CMysettingsDialog.h"

class dsp_paulstretch : public dsp_impl_base
{
public:

	dsp_paulstretch(const dsp_preset& preset) :
		myLastSeenNumberOfChannels(-1), 
		myLastSeenSampleRate(-1), 
		myLastSeenChannelConfig(-1),
		myPaulstretchPreset()
	{
		myLastTimeWindowSizeChanged = std::chrono::system_clock::now() - std::chrono::hours(1);
		if (!readPreset(preset))
			pfc::outputDebugLine("Failed to read preset - paulstretchDSP.h constructor.");
	}

	static GUID g_get_guid() {
		return paulstretchPreset::getGUID();
	}

	static bool g_have_config_popup() {
		return true;
	}

	GUID get_owner()
	{
		return g_get_guid();
	}

	static void g_get_name(pfc::string_base & p_out) { p_out = "Paulstretch DSP"; }

	static bool g_get_default_preset(dsp_preset & preset)
	{
		paulstretchPreset paulstretchPreset;
		paulstretchPreset.writeData(preset);

		return true;
	}

	static void g_show_config_popup(const dsp_preset & p_data, HWND p_parent, dsp_preset_edit_callback & p_callback)
	{
		std::function<void(paulstretchPreset)> lambda = [&](paulstretchPreset data) -> void {
			dsp_preset_impl newPreset;
			data.writeData(newPreset);
			p_callback.on_preset_changed(newPreset);
		};
		CMySettingsDialog myDialog(p_data, lambda);
		myDialog.DoModal(p_parent);
	}

	bool on_chunk(audio_chunk * chunk, abort_callback &) {

		if (!myPaulstretchPreset.enabled())
			return true;

		// All state required to do paulstretch is saved after this call, so all 'myLastSeen...' 
		// variable usage is fresh.
		//
		remember_state(chunk);
		splitAndFeed(chunk);
		while(canStretch())
			stretch(myPaulstretchPreset.stretchAmount());
		
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
		if (myLastSeenNumberOfChannels <= 0 || myLastSeenNumberOfChannels != myPaulstretch.size())
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
		if (myLastSeenWindowSize != myPaulstretchPreset.windowSize())
		{
			std::chrono::system_clock::time_point current_time_point = std::chrono::system_clock::now();
			auto duration = current_time_point - myLastTimeWindowSizeChanged;
			if (duration > std::chrono::seconds(1)) {
				myLastSeenWindowSize = myPaulstretchPreset.windowSize();
				myLastTimeWindowSizeChanged = current_time_point;
				resizePaulstretch(chunk, chunk->get_channels(), myPaulstretchPreset.windowSize());
			}
		}
		if (myLastSeenNumberOfChannels != chunk->get_channels())
		{
			resizePaulstretch(chunk, chunk->get_channels(), myPaulstretchPreset.windowSize());
		}
		else if (myLastSeenSampleRate != chunk->get_sample_rate())
		{
			resizePaulstretch(chunk, chunk->get_channels(), myPaulstretchPreset.windowSize());
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

	void on_endofplayback(abort_callback & callback)
	{
		on_endoftrack(callback);
	}

	void on_endoftrack(abort_callback &) {

		if (!myPaulstretchPreset.enabled())
			return;
		if (myPaulstretch.empty())
			return;

		// For paulstretch, we need to pad with 0s for the last window to process.
		// How much padding we need depends on how much data we are buffering.
		//					 
		float stretchAmount = myPaulstretchPreset.stretchAmount();
		size_t numRequiredStretches = myPaulstretch[0].finalStretchesRequired(stretchAmount);

		for (size_t numStretches = 0; numStretches < numRequiredStretches; numStretches++) {
			for (size_t i = 0; i < myPaulstretch.size(); i++)
			{
				while (!myPaulstretch[i].canStep())
					myPaulstretch[i].feed(0.0f);
			}
			stretch(stretchAmount);
		}

		for (size_t i = 0; i < myPaulstretch.size(); i++)
			myPaulstretch[i].flush();
	}

	// If you have any audio data buffered, you should drop it immediately and reset the DSP to a freshly initialized state.
	// Called after a seek etc.
	//
	void flush() {
		// We could flush, but I think the gradual fade introduced by not doing so sounds nice.
	}

	double get_latency() {
		// If the DSP buffers some amount of audio data, it should return the duration of buffered data (in seconds) here.
		if(myPaulstretchPreset.enabled())
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

	bool readPreset(const dsp_preset& preset)
	{
		paulstretchPreset paulstretchPreset;
		if (!paulstretchPreset.readData(preset))
			return false;
		myPaulstretchPreset = paulstretchPreset;
		return true;
	}
	
	float myLastSeenWindowSize;
	size_t myLastSeenNumberOfChannels;
	size_t myLastSeenSampleRate;
	size_t myLastSeenChannelConfig;
	paulstretchPreset myPaulstretchPreset;

	std::chrono::system_clock::time_point myLastTimeWindowSizeChanged;
	std::vector<NewPaulstretch> myPaulstretch;
};

#endif

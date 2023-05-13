#pragma once

#include <SDK/foobar2000-lite.h>
#include <helpers/dsp_dialog.h>
#include <queue>
#include <chrono>

#include "paulstretch.h"
#include "paulstretch_preset.h"
#include "paulstretch_dialog.h"

namespace pauldsp {

	class dsp_paulstretch : public dsp_impl_base_t<dsp_v3>
	{
	public:

		dsp_paulstretch(const dsp_preset& preset) :
			myLastSeenNumberOfChannels(0),
			myLastSeenSampleRate(0),
			myLastSeenChannelConfig(0),
			myPaulstretchPreset(),
			myHasSeenChunk(false),
			myKissFFTR(2, false),
			myKissFFTRI(2, true)
		{
			if (!readPreset(preset))
				pfc::outputDebugLine("Failed to read preset - paulstretchDSP.h constructor.");
		}

		static GUID g_get_guid() {
			return paulstretch_preset::getGUID();
		}

		static bool g_have_config_popup() {
			return true;
		}

		GUID get_owner()
		{
			return g_get_guid();
		}

		static void g_get_name(pfc::string_base& p_out) { p_out = "Paulstretch DSP"; }

		static bool g_get_default_preset(dsp_preset& preset)
		{
			paulstretch_preset paulstretchPreset;
			paulstretchPreset.writeData(preset);

			return true;
		}

		static void g_show_config_popup(const dsp_preset& p_data, HWND p_parent, dsp_preset_edit_callback& p_callback)
		{
			std::function<void(paulstretch_preset)> lambda = [&](paulstretch_preset data) -> void {
				dsp_preset_impl newPreset;
				data.writeData(newPreset);
				p_callback.on_preset_changed(newPreset);
			};
			paulstretch_dialog myDialog(p_data, lambda);
			myDialog.DoModal(p_parent);
		}

		bool apply_preset(const dsp_preset& preset)
		{
			return readPreset(preset);
		}

		bool on_chunk(audio_chunk* chunk, abort_callback& callback) {

			if (!myPaulstretchPreset.enabled())
				return true;

			// All state required to do paulstretch is saved after this call, so all 'myLastSeen...' 
			// variable usage is fresh.
			//
			remember_state(chunk);
			splitAndFeed(chunk);
			while (canStretch() && !callback.is_aborting())
				stretch(myPaulstretchPreset.stretchAmount());

			// We need to buffer chunks on our own, so drop everything.
			//
			return false;
		}

		bool canStretch()
		{
			return !myPaulstretch.empty() && canAllStep();
		}

		void stretch(const double stretch_amount)
		{
			std::vector<AudioBuffer*> output(myLastSeenNumberOfChannels);
			for (size_t i = 0; i < myLastSeenNumberOfChannels; i++)
				output[i] = myPaulstretch[i].step(stretch_amount, myKissFFTR, myKissFFTRI);
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
			new_chunk->set_data(
				new_audio_sample.get(),
				results[0]->size(),
				static_cast<unsigned int>(myLastSeenNumberOfChannels),
				static_cast<unsigned int>(myLastSeenSampleRate),
				static_cast<unsigned int>(myLastSeenChannelConfig)
			);
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
			if (myLastSeenNumberOfChannels != chunk->get_channels())
			{
				resizePaulstretch(chunk, chunk->get_channels(), myPaulstretchPreset.windowSize());
			}
			else if (myLastSeenSampleRate != chunk->get_sample_rate())
			{
				resizePaulstretch(chunk, chunk->get_channels(), myPaulstretchPreset.windowSize());
			}
			// Because we can change settings on the fly now (apply_reset call), we need to check window
			// size changes.
			else if (myLastSeenWindowSize != myPaulstretchPreset.windowSize())
			{
				resizePaulstretch(chunk, chunk->get_channels(), myPaulstretchPreset.windowSize());
			}

			myLastSeenNumberOfChannels = chunk->get_channels();
			myLastSeenSampleRate = chunk->get_sample_rate();
			myLastSeenChannelConfig = chunk->get_channel_config();
			myLastSeenWindowSize = myPaulstretchPreset.windowSize();
			myHasSeenChunk = true;
		}

		void resizePaulstretch(audio_chunk* chunk, size_t n_channels, const double window_size)
		{
			while (myPaulstretch.size() > n_channels)
				myPaulstretch.pop_back();
			while (myPaulstretch.size() < n_channels)
				myPaulstretch.push_back(NewPaulstretch(window_size, chunk->get_sample_rate()));
			for (size_t i = 0; i < myPaulstretch.size(); i++)
				myPaulstretch[i].resize(window_size, chunk->get_sample_rate());

			if (myPaulstretch.empty() || window_size <= 0.0)
				return;

			size_t windowSizeInSamples = myPaulstretch[0].windowSize();
			myKissFFTR = kissfft<audio_sample>(windowSizeInSamples >> 1, false);
			myKissFFTRI = kissfft<audio_sample>(windowSizeInSamples >> 1, true);
		}

		void on_endofplayback(abort_callback& callback)
		{
			if (myPaulstretchPreset.isConversion())
				on_endoftrack(callback);
		}

		void on_endoftrack(abort_callback& callback) {

			if (callback.is_aborting())
				return;
			if (!myPaulstretchPreset.enabled())
				return;
			if (myPaulstretch.empty())
				return;

			// We need to pad with 0s for the last window to process.
			// How much padding we need depends on how much data we are buffering.
			//					 
			double stretchAmount = myPaulstretchPreset.stretchAmount();
			size_t numRequiredStretches = myPaulstretch[0].finalStretchesRequired(stretchAmount);

			for (size_t numStretches = 0; numStretches < numRequiredStretches; numStretches++) {
				for (size_t i = 0; i < myPaulstretch.size() && !callback.is_aborting(); i++)
				{
					if (!myPaulstretch[i].canStep())
						myPaulstretch[i].feedUntilStep(0);

					// just a sanity check for the previous call.
					while (!myPaulstretch[i].canStep())
						myPaulstretch[i].feed(0);
				}
				if (callback.is_aborting())
					return;
				stretch(stretchAmount);
			}

			for (size_t i = 0; i < myPaulstretch.size(); i++)
				myPaulstretch[i].flush();
		}

		// If you have any audio data buffered, you should drop it immediately and reset the DSP to a freshly initialized state.
		// Called after a seek etc.
		//
		void flush() {
			for (size_t i = 0; i < myPaulstretch.size(); i++)
				myPaulstretch[i].flush();
		}

		double get_latency() {

			return 0;
			// If the DSP buffers some amount of audio data, it should return the duration of buffered data (in seconds) here.
			if (!myHasSeenChunk)
				return 0;

			if (!myPaulstretchPreset.enabled() || myPaulstretch.empty() || myLastSeenSampleRate == 0)
				return 0;

			double latency = myPaulstretch[0].numBufferedSamples() * (1.0 / myLastSeenSampleRate);
			if (latency > 5)
				return 5;
			return latency;
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
			paulstretch_preset paulstretchPreset;
			if (!paulstretchPreset.readData(preset))
				return false;
			myPaulstretchPreset = paulstretchPreset;
			return true;
		}

		bool myHasSeenChunk;
		size_t myLastSeenNumberOfChannels;
		size_t myLastSeenSampleRate;
		size_t myLastSeenChannelConfig;
		double myLastSeenWindowSize;
		paulstretch_preset myPaulstretchPreset;

		std::vector<NewPaulstretch> myPaulstretch;
		kissfft<audio_sample> myKissFFTR;
		kissfft<audio_sample> myKissFFTRI;
	};
}
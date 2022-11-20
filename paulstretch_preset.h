#pragma once
#include "dumb_fraction.h"

namespace pauldsp {

	struct paulstretch_preset
	{
	public:

		enum REPLACE_RESULT
		{
			UPDATED,
			NOT_FOUND,
			NO_UPDATE_NEEDED
		};

		/**
		 * \brief replaces the current preset with the given one. If the given preset does not
		 *        have different values than the current one, nothing is done.
		 *
		 * \param newDspPreset the preset to overwrite the currently active one.
		 * \return true if given preset was different than current one setting-by-setting and
		 *          successfully overwrote the current preset, false otherwise.
		 */
		static REPLACE_RESULT replaceData(const dsp_preset& newDspPreset)
		{
			dsp_chain_config_impl dspChainConfig;
			static_api_ptr_t<dsp_config_manager> dspConfigManager = static_api_ptr_t<dsp_config_manager>();
			dspConfigManager->get_core_settings(dspChainConfig);
			for (size_t i = 0; i < dspChainConfig.get_count(); i++)
			{
				dsp_preset_impl dspPreset = dspChainConfig.get_item(i);
				if (dspPreset.get_owner() != newDspPreset.get_owner())
					continue;

				if (!newSettings(newDspPreset, dspPreset))
					return NO_UPDATE_NEEDED;
				
				if (newSettings(newDspPreset, dspPreset))
				{
					dspChainConfig.replace_item(newDspPreset, i);
					dspConfigManager->set_core_settings(dspChainConfig);
					return UPDATED;
				}
			}

			return NOT_FOUND;
		}

	private:
		static bool newSettings(const dsp_preset& lhpreset, const dsp_preset& rhpreset)
		{
			return lhpreset != rhpreset;
		}

	public:
		double myStretchAmount;
		double myWindowSize;
		bool myEnabled;
		bool myIsConversion;
		Fraction myMaxStretch;
		Fraction myMinStretch;
		Fraction myMaxWindow;
		Fraction myMinWindow;
		Fraction myStretchPrecision;
		Fraction myWindowPrecision;

		static const GUID getGUID()
		{
			// {1408BA62-28E2-4ACC-9C3B-98AF83FBBE96}
			static const GUID guid =
			{ 0x1408ba62, 0x28e2, 0x4acc,{ 0x9c, 0x3b, 0x98, 0xaf, 0x83, 0xfb, 0xbe, 0x96 } };
			return guid;
		}

		paulstretch_preset(const dsp_preset& preset)
		{
			readData(preset);
		}

		paulstretch_preset(
			const double stretchAmount = 4.0f,
			const double windowSize = 0.28f,
			const bool enabled = false,
			const bool isConversion = false,
			const Fraction maxStretch = Fraction(40),
			const Fraction minStretch = Fraction(1),
			const Fraction maxWindow = Fraction(2),
			const Fraction minWindow = Fraction(1, 100),
			const Fraction stretchPrecision = Fraction(1, 10),
			const Fraction windowPrecision = Fraction(1, 100)
		)
		{
			myStretchAmount = stretchAmount;
			myWindowSize = windowSize;
			myEnabled = enabled;
			myIsConversion = isConversion;
			myMaxStretch = maxStretch;
			myMinStretch = minStretch;
			myMaxWindow = maxWindow;
			myMinWindow = minWindow;
			myStretchPrecision = stretchPrecision;
			myWindowPrecision = windowPrecision;
		}

		bool enabled() const
		{
			return myEnabled;
		}

		bool isConversion() const
		{
			return myIsConversion;
		}

		double windowSize() const
		{
			return myWindowSize;
		}

		double stretchAmount() const
		{
			return myStretchAmount;
		}

		void writeData(dsp_preset& out)
		{
			dsp_preset_builder builder;
			builder << myStretchAmount;
			builder << myWindowSize;
			builder << myEnabled;
			builder << myIsConversion;
			builder << myMaxStretch.getNumerator();
			builder << myMaxStretch.getDenominator();
			builder << myMinStretch.getNumerator();
			builder << myMinStretch.getDenominator();
			builder << myMaxWindow.getNumerator();
			builder << myMaxWindow.getDenominator();
			builder << myMinWindow.getNumerator();
			builder << myMinWindow.getDenominator();
			builder << myStretchPrecision.getNumerator();
			builder << myStretchPrecision.getDenominator();
			builder << myWindowPrecision.getNumerator();
			builder << myWindowPrecision.getDenominator();
			builder.finish(getGUID(), out);
		}

		bool readData(const dsp_preset& in)
		{
			if (in.get_owner() != getGUID())
				return false;
			try
			{
				int64_t numerator;
				uint64_t denominator;
				dsp_preset_parser parser(in);
				parser >> myStretchAmount;
				parser >> myWindowSize;
				parser >> myEnabled;
				parser >> myIsConversion;
				parser >> numerator;
				parser >> denominator;
				myMaxStretch = Fraction(numerator, denominator);
				parser >> numerator;
				parser >> denominator;
				myMinStretch = Fraction(numerator, denominator);
				parser >> numerator;
				parser >> denominator;
				myMaxWindow = Fraction(numerator, denominator);
				parser >> numerator;
				parser >> denominator;
				myMinWindow = Fraction(numerator, denominator);
				parser >> numerator;
				parser >> denominator;
				myStretchPrecision = Fraction(numerator, denominator);
				parser >> numerator;
				parser >> denominator;
				myWindowPrecision = Fraction(numerator, denominator);
			}
			catch (exception_io_data)
			{
				*this = paulstretch_preset();
				return false;
			}

			validate();
			return true;
		}

		template<class T>
		T clamp(const T minVal, const T val, const T maxVal)
		{
			return max(minVal, min(val, maxVal));
		}

		void validate()
		{
			myMinStretch = clamp(Fraction(1, 2), myMinStretch, Fraction(1));
			myMaxStretch = clamp(Fraction(8), myMaxStretch, Fraction(100));
			myMinWindow = clamp(Fraction(1, 100), myMinWindow, Fraction(1, 10));
			myMaxWindow = clamp(Fraction(1), myMaxWindow, Fraction(5));
			myStretchAmount = clamp(0.5, myStretchAmount, 100.0);
			myWindowSize = clamp(0.01, myWindowSize, 5.0);
			myStretchPrecision = clamp(Fraction(1, 1000), myStretchPrecision, Fraction(1));
			myWindowPrecision = clamp(Fraction(1, 1000), myWindowPrecision, Fraction(1, 10));
		}
	};
}
#pragma once

struct paulstretch_preset
{
public:
	/**
	 * \brief replaces the current preset with the given one. If the given preset does not
	 *        have different values than the current one, nothing is done.
	 *
	 * \param newDspPreset the preset to overwrite the currently active one.
	 * \return true if given preset was different than current one setting-by-setting and
	 *          successfully overwrote the current preset, false otherwise.
	 */
	static bool replaceData(const dsp_preset& newDspPreset)
	{
		dsp_chain_config_impl dspChainConfig;
		static_api_ptr_t<dsp_config_manager> dspConfigManager = static_api_ptr_t<dsp_config_manager>();
		dspConfigManager->get_core_settings(dspChainConfig);
		for (size_t i = 0; i < dspChainConfig.get_count(); i++)
		{
			dsp_preset_impl dspPreset = dspChainConfig.get_item(i);
			if (dspPreset.get_owner() == newDspPreset.get_owner() && newSettings(newDspPreset, dspPreset))
			{
				dspChainConfig.replace_item(newDspPreset, i);
				dspConfigManager->set_core_settings(dspChainConfig);
				return true;
			}
		}

		return false;
	}
private:
	static bool newSettings(const dsp_preset& lhpreset, const dsp_preset& rhpreset)
	{
		paulstretch_preset lh(lhpreset);
		paulstretch_preset rh(rhpreset);
		if (lh.enabled() != rh.enabled())
			return true;
		if (lh.windowSize() != rh.windowSize())
			return true;
		if (lh.stretchAmount() != rh.stretchAmount())
			return true;

		return false;
	}

private:
	struct paulstretchData
	{
		paulstretchData(float stretchAmount, float windowSize, float enabled)
		{
			this->stretchAmount = stretchAmount;
			this->windowSize = windowSize;
			this->enabled = enabled;
		}
		float stretchAmount;
		float windowSize;
		bool enabled;
	};

public:
	float myStretchAmount;
	float myWindowSize;
	float myEnabled;

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

	paulstretch_preset(const float stretchAmount = 4.0f, const float windowSize = 0.28f, const bool enabled = false)
	{
		myStretchAmount = stretchAmount;
		myWindowSize = windowSize;
		myEnabled = enabled;
	}

	bool enabled() const
	{
		return myEnabled;
	}

	float windowSize() const
	{
		return myWindowSize;
	}

	float stretchAmount() const
	{
		return myStretchAmount;
	}

	void writeData(dsp_preset& out)
	{
		paulstretchData data = constructDataStruct();
		out.set_owner(getGUID());
		out.set_data(&data, sizeof(data));
	}

	bool readData(const dsp_preset& in)
	{
		if (in.get_owner() != getGUID())
			return false;
		if (in.get_data_size() != sizeof(paulstretchData))
			return false;
		paulstretchData castedData = *(paulstretchData*)in.get_data();
		readDataStruct(castedData);
		return true;
	}

private:

	void readDataStruct(paulstretchData data)
	{
		myStretchAmount = data.stretchAmount;
		myWindowSize = data.windowSize;
		myEnabled = data.enabled;
	}

	paulstretchData constructDataStruct()
	{
		return constructDataStruct(myStretchAmount, myWindowSize, myEnabled);
	}

	paulstretchData constructDataStruct(float stretchAmount, float windowSize, bool enabled) const
	{
		return paulstretchData(stretchAmount, windowSize, enabled);
	}
};
#pragma once
#include <SDK/foobar2000-lite.h>
#include "paulstretch_preset.h"
#include <optional>

namespace pauldsp {

	class paulstretch_menu : public mainmenu_commands
	{
	private:
		::std::optional<dsp_config_manager::ptr> myConfigManager;
	public:

		enum
		{
			// cmd_stretch_settings = 0,
			cmd_stretch_enable,
			cmd_total
		};

		t_uint32 get_command_count() override;

		GUID get_command(t_uint32 p_index) override;

		void get_name(t_uint32 p_index, pfc::string_base& p_out) override;

		bool get_description(t_uint32 p_index, pfc::string_base& p_out) override;

		GUID get_parent() override;

		void execute(t_uint32 p_index, service_ptr_t<service_base> p_callback) override;

		bool get_display(t_uint32 p_index, pfc::string_base& p_text, t_uint32& p_flags) override;

		//void startMenu();

	private:
		void togglePaulstretch();

		dsp_config_manager::ptr getConfigManager()
		{
			if (!myConfigManager)
				myConfigManager = dsp_config_manager::get();
			return myConfigManager.value();
		}

		::std::optional<paulstretch_preset> queryPreset()
		{
			dsp_preset_impl dspPreset;
			if (getConfigManager()->core_query_dsp(paulstretch_preset::getGUID(), dspPreset))
			{
				return ::std::optional(paulstretch_preset(dspPreset));
			}

			return ::std::nullopt;
		}
	};
}
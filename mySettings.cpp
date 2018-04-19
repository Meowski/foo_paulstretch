#include "stdafx.h"
#include "mySettings.h"
#include "CMysettingsDialog.h"

static const GUID g_mysettings_guid = { 0x830a5a98, 0xccac, 0x4539,{ 0xa3, 0x23, 0x64, 0x6a, 0xc2, 0x29, 0xe1, 0xbe } };
static mainmenu_group_popup_factory g_mainmenu_group(g_mysettings_guid, mainmenu_groups::playback, mainmenu_commands::sort_priority_dontcare, "Paulstretch");

t_uint32 mySettings::get_command_count()
{
	return cmd_total;
}

GUID mySettings::get_command(t_uint32 p_index)
{
	static GUID mySettings_guid = { 0xe1a3b87f, 0x61a7, 0x4989,{ 0x9c, 0xa6, 0x5e, 0x89, 0xcf, 0x8, 0x86, 0xfc } };
	static GUID my_enabled_guid = { 0xa0d9b939, 0xf3d8, 0x40c1,{ 0x9a, 0xf1, 0xa2, 0xae, 0xa5, 0xde, 0xe2, 0xa8 } };

	switch (p_index)
	{
	case cmd_stretch_settings: return mySettings_guid;
	case cmd_stretch_enable: return my_enabled_guid;
	default: uBugCheck();
	}
}

void mySettings::get_name(t_uint32 p_index, pfc::string_base& p_out)
{
	switch (p_index)
	{
	case cmd_stretch_settings: p_out = "Paulstretch Settings";
		break;
	case cmd_stretch_enable: p_out = "Toggle Paulstretch";
		break;
	default: uBugCheck();
	}
}

bool mySettings::get_description(t_uint32 p_index, pfc::string_base& p_out)
{
	switch (p_index)
	{
	case cmd_stretch_settings: p_out = "Display Paulstretch Settings";
		return true;
	case cmd_stretch_enable: p_out = "Toggle Paulstretch On and Off";
		return true;
	default: uBugCheck();
	}
}

GUID mySettings::get_parent()
{
	return g_mysettings_guid;
}

void mySettings::execute(t_uint32 p_index, service_ptr_t<service_base> p_callback)
{
	switch (p_index)
	{
	case cmd_stretch_settings:
		startMenu();
		break;
	case cmd_stretch_enable: togglePaulstretch();
		break;
	default:
		uBugCheck();
	}
}

void mySettings::startMenu()
{
	try
	{
		// ImplementModelessTracking registers our dialog to receive dialog messages thru main app loop's IsDialogMessage().
		// CWindowAutoLifetime creates the window in the constructor (taking the parent window as a parameter) and deletes the object when the window has been destroyed (through WTL's OnFinalMessage).
		new CWindowAutoLifetime<ImplementModelessTracking<CMySettingsDialog>>(core_api::get_main_window());
	}
	catch (std::exception const& e)
	{
		popup_message::g_complain("Dialog creation failure", e);
	}
}

void mySettings::togglePaulstretch()
{
	static_api_ptr_t<dsp_config_manager> dspConfigManager = static_api_ptr_t<dsp_config_manager>();
	dsp_preset_impl dspPreset;
	if (dspConfigManager->core_query_dsp(paulstretchPreset::getGUID(), dspPreset))
	{
		paulstretchPreset preset(dspPreset);
		preset.myEnabled = !preset.myEnabled;
		preset.writeData(dspPreset);
		fb2k::inMainThread([=]()
		{
			paulstretchPreset::replaceData(dspPreset);
		});
		
	}
}

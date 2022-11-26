#include "stdafx.h"
#include "paulstretch_menu.h"
#include "paulstretch_dialog.h"

using namespace pauldsp;

static const GUID g_paulstretch_menu_guid = { 0x830a5a98, 0xccac, 0x4539,{ 0xa3, 0x23, 0x64, 0x6a, 0xc2, 0x29, 0xe1, 0xbe } };
static mainmenu_group_factory g_mainmenu_group(g_paulstretch_menu_guid, mainmenu_groups::playback, mainmenu_commands::sort_priority_dontcare);

t_uint32 paulstretch_menu::get_command_count()
{
	return cmd_total;
}

GUID paulstretch_menu::get_command(t_uint32 p_index)
{
//	static GUID mySettings_guid = { 0xe1a3b87f, 0x61a7, 0x4989,{ 0x9c, 0xa6, 0x5e, 0x89, 0xcf, 0x8, 0x86, 0xfc } };
	static GUID my_enabled_guid = { 0xa0d9b939, 0xf3d8, 0x40c1,{ 0x9a, 0xf1, 0xa2, 0xae, 0xa5, 0xde, 0xe2, 0xa8 } };

	switch (p_index)
	{
//	case cmd_stretch_settings: return mySettings_guid;
	case cmd_stretch_enable: return my_enabled_guid;
	default: uBugCheck();
	}
}

void paulstretch_menu::get_name(t_uint32 p_index, pfc::string_base& p_out)
{
	switch (p_index)
	{
//	case cmd_stretch_settings: p_out = "Paulstretch Settings";
//		break;
	case cmd_stretch_enable: p_out = "Paulstretch Toggle";
		break;
	default: uBugCheck();
	}
}

bool paulstretch_menu::get_description(t_uint32 p_index, pfc::string_base& p_out)
{
	switch (p_index)
	{
//	 case cmd_stretch_settings: p_out = "Display Paulstretch Settings";
//    	return true;
	case cmd_stretch_enable: p_out = "Toggle Paulstretch On and Off";
		return true;
	default: uBugCheck();
	}
}

GUID paulstretch_menu::get_parent()
{
	return g_paulstretch_menu_guid;
}

void paulstretch_menu::execute(t_uint32 p_index, service_ptr_t<service_base> p_callback)
{
	switch (p_index)
	{
//	case cmd_stretch_settings:
//		startMenu();
		break;
	case cmd_stretch_enable: togglePaulstretch();
		break;
	default:
		uBugCheck();
	}
}

/*
void paulstretch_menu::startMenu()
{
	try
	{
		// ImplementModelessTracking registers our dialog to receive dialog messages thru main app loop's IsDialogMessage().
		// CWindowAutoLifetime creates the window in the constructor (taking the parent window as a parameter) and deletes the object when the window has been destroyed (through WTL's OnFinalMessage).
		fb2k::newDialog<paulstretch_dialog>();
	}
	catch (std::exception const& e)
	{
		popup_message::g_complain("Dialog creation failure", e);
	}
}
*/

bool paulstretch_menu::get_display(t_uint32 p_index, pfc::string_base& p_text, t_uint32& p_flags)
{
	auto optPreset = queryPreset();
	if (!optPreset.has_value())
		p_flags = 0;
	else
		(*optPreset).enabled() ? p_flags = menu_flags::checked : 0;
	get_name(p_index, p_text);
	return true;
}

void paulstretch_menu::togglePaulstretch()
{
	auto optPreset = queryPreset();
	if (optPreset.has_value())
	{
		paulstretch_preset preset = *optPreset;
		preset.myEnabled = !preset.myEnabled;
		dsp_preset_impl asDSPPreset = preset.toPreset();
		fb2k::inMainThread([=]() {
			paulstretch_preset::replaceData(asDSPPreset);
		});

	}
	else
	{
		// If it wasn't present, we'll enable it here.
		paulstretch_preset paul_preset = paulstretch_preset();
		paul_preset.myEnabled = true;
		dsp_preset_impl preset_impl;
		paul_preset.writeData(preset_impl);	
		getConfigManager()->core_enable_dsp(preset_impl, dsp_config_manager::default_insert_last);
	}
}

#pragma once
#include <SDK/foobar2000.h>

class paulstretch_menu : public mainmenu_commands
{
public:
	enum
	{
		cmd_stretch_settings = 0,
		cmd_stretch_enable,
		cmd_total
	};

	t_uint32 get_command_count();

	GUID get_command(t_uint32 p_index);

	void get_name(t_uint32 p_index, pfc::string_base& p_out);

	bool get_description(t_uint32 p_index, pfc::string_base& p_out);

	GUID get_parent();

	void execute(t_uint32 p_index, service_ptr_t<service_base> p_callback);

	void startMenu();

private:
	void togglePaulstretch();
};
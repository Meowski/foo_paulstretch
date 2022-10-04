#include "stdafx.h"
#include <SDK/foobar2000.h>
#include <helpers/helpers.h>
#include "paulstretch_menu.h"
#include "paulstretch_dsp.h"

// Declaration of your component's version information
// Since foobar2000 v1.0 having at least one of these in your DLL is mandatory to let the troubleshooter tell different versions of your component apart.
// Note that it is possible to declare multiple components within one DLL, but it's strongly recommended to keep only one declaration per DLL.
// As for 1.1, the version numbers are used by the component update finder to find updates; for that to work, you must have ONLY ONE declaration per DLL. If there are multiple declarations, the component is assumed to be outdated and a version number of "0" is assumed, to overwrite the component with whatever is currently on the site assuming that it comes with proper version numbers.
DECLARE_COMPONENT_VERSION(
	"Paulstretch",
	"1.1",
	R"(Applies the paulstretch algorithm during playback.

https://github.com/Meowski/foo_paulstretch)");


// This will prevent users from renaming your component around (important for proper troubleshooter behaviors) or loading multiple instances of it.
VALIDATE_COMPONENT_FILENAME("foo_dsp_paulstretch.dll");

// Use dsp_factory_nopreset_t<> instead of dsp_factory_t<> if your DSP does not provide preset/configuration functionality.
static dsp_factory_t<dsp_paulstretch> g_dsp_sample_factory;
static mainmenu_commands_factory_t<paulstretch_menu> g_mainmenu_paulstretch_settings;

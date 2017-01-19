#include "stdafx.h"
#include "resource.h"
#include "paulstretchDSP.h"


static const GUID g_mysettings_guid = { 0x830a5a98, 0xccac, 0x4539,{ 0xa3, 0x23, 0x64, 0x6a, 0xc2, 0x29, 0xe1, 0xbe } };
static mainmenu_group_popup_factory g_mainmenu_group(g_mysettings_guid, mainmenu_groups::playback, mainmenu_commands::sort_priority_dontcare, "Paulstretch");

void StartMenu();

class my_settings : public mainmenu_commands
{
public:
	enum
	{
		cmd_stretch_settings = 0,
		cmd_total
	};

	t_uint32 get_command_count() {
		return cmd_total;
	}

	GUID get_command(t_uint32 p_index) {
		static GUID my_settings_guid = { 0xe1a3b87f, 0x61a7, 0x4989,{ 0x9c, 0xa6, 0x5e, 0x89, 0xcf, 0x8, 0x86, 0xfc } };
		switch (p_index)
		{
		case cmd_stretch_settings: return my_settings_guid; 
		default: uBugCheck();
		}
	}

	void get_name(t_uint32 p_index, pfc::string_base & p_out) {
		switch (p_index) {
		case cmd_stretch_settings: p_out = "Paulstretch Settings"; break;
		default: uBugCheck();
		}
	}

	bool get_description(t_uint32 p_index, pfc::string_base & p_out) {
		switch (p_index) {
		case cmd_stretch_settings: p_out = "Set commands for Paulstretch."; return true;
		default: uBugCheck();
		}
	}

	GUID get_parent() {
		return g_mysettings_guid;
	}

	void execute(t_uint32 p_index, service_ptr_t<service_base> p_callback) {
		switch (p_index) {
		case cmd_stretch_settings:
			::StartMenu();
			break;
		default:
			uBugCheck();
		}
	}

};

static mainmenu_commands_factory_t<my_settings> g_mainmenu_paulstretch_settings;

class CMySettingsDialog : public CDialogImpl<CMySettingsDialog>
{
public:
	static int myCurrentStretchPos;
	static int myCurrentWindowPos;
	static bool myEnabled;

	enum
	{
		IDD = IDD_SETTINGS
	};

	BEGIN_MSG_MAP(CMySettingsDialog)
		MSG_WM_INITDIALOG(OnInitDialog)	 
		COMMAND_HANDLER_EX(IDCANCEL, BN_CLICKED, OnCancel)
		COMMAND_HANDLER_EX(IDC_ENABLE_STRETCH, BN_CLICKED, OnEnabledCheckBoxChanged)
		MSG_WM_HSCROLL(OnHScroll)
		NOTIFY_HANDLER(IDC_STRETCH_SLIDER, NM_CUSTOMDRAW, OnNMCustomdrawStretchSlider)
	END_MSG_MAP()

	BOOL OnInitDialog(CWindow, LPARAM)
	{
		myStretchSlider = GetDlgItem(IDC_STRETCH_SLIDER);
		myWindowSizeSlider = GetDlgItem(IDC_WINDOW_SIZE_SLIDER);
		myEnabledCheckBox = GetDlgItem(IDC_ENABLE_STRETCH);

		myStretchSlider.SetRange(ourStretchMin, ourStretchMax);
		myWindowSizeSlider.SetRange(ourWindowSizeMin, ourWindowSizeMax);

		myStretchSlider.SetPos(myCurrentStretchPos);
		myWindowSizeSlider.SetPos(myCurrentWindowPos);

		myEnabledCheckBox.SetCheck(CMySettingsDialog::myEnabled);

		updateStretch(myCurrentStretchPos);
		updateWindow(myCurrentWindowPos);
		checkIfPaulstretchIsEnabled();

		::ShowWindowCentered(static_cast<CWindow>(*this), GetParent());
		return TRUE;
	}

	void OnCancel(UINT, int, CWindow) {
		DestroyWindow();
	}

	void OnEnabledCheckBoxChanged(UINT, int, CWindow)
	{
		CMySettingsDialog::myEnabled = myEnabledCheckBox.GetCheck() == BST_CHECKED ? true : false;
		dsp_paulstretch::enabled = myEnabled;
	}

	void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar pScrollBar) const
	{
		switch(pScrollBar.GetDlgCtrlID())
		{
		case IDC_STRETCH_SLIDER: updateStretch(myStretchSlider.GetPos()); break;
		case IDC_WINDOW_SIZE_SLIDER: updateWindow(myWindowSizeSlider.GetPos()); break;
		default: uBugCheck();
		}
			
	}

	void updateStretch(int value) const
	{
		pfc::string_formatter msg; msg << "Stretch Amount: " << pfc::format_float(value / (1.0f * ourStretchMin), 0, 1);
		uSetDlgItemText(*this, IDC_STRETCH_LABEL, msg);
		myCurrentStretchPos = value;
		dsp_paulstretch::stretch_amount = value / (1.0f * ourStretchMin);
	}

	void updateWindow(int value) const
	{
		pfc::string_formatter msg; msg << "Window Size: " << pfc::format_float(value / 1000.0f, 0, 2) << " s";
		uSetDlgItemText(*this, IDC_WINDOW_SIZE_LABEL, msg);
		myCurrentWindowPos = value;
		dsp_paulstretch::window_size = value / 1000.0f;
	}

	void checkIfPaulstretchIsEnabled()
	{
		if (isPaulstretchEnabled())
			return;

		pfc::string_formatter msg;
		msg << "Paulstretch DSP must first be enabled in preferences.";
		uSetDlgItemText(*this, IDC_ENABLED_LABEL, msg);
	}

private:

	bool isPaulstretchEnabled()
	{
		myDSP_manager_ptr = static_api_ptr_t<dsp_config_manager>();
		dsp_preset_impl out;
		return myDSP_manager_ptr.get_ptr()->core_query_dsp(dsp_paulstretch::g_get_guid(), out);
	}

	CTrackBarCtrl myStretchSlider;
	CTrackBarCtrl myWindowSizeSlider;
	CButton myEnabledCheckBox;
	const static int ourStretchMin = 10;
	const static int ourDefaultStretch = 40;
	const static int ourStretchMax = 400;
	const static int ourWindowSizeMin = 16;
	const static int ourDefaultWindowSize = 280;
	const static int ourWindowSizeMax = 2000;

	static_api_ptr_t<dsp_config_manager> myDSP_manager_ptr;
public:
	LRESULT OnNMCustomdrawStretchSlider(int /*idCtrl*/, LPNMHDR pNMHDR, BOOL& /*bHandled*/);
};

int CMySettingsDialog::myCurrentStretchPos = ourDefaultStretch;
int CMySettingsDialog::myCurrentWindowPos = ourDefaultWindowSize;
bool CMySettingsDialog::myEnabled = false;

void StartMenu() {
	try {
		// ImplementModelessTracking registers our dialog to receive dialog messages thru main app loop's IsDialogMessage().
		// CWindowAutoLifetime creates the window in the constructor (taking the parent window as a parameter) and deletes the object when the window has been destroyed (through WTL's OnFinalMessage).
		new CWindowAutoLifetime<ImplementModelessTracking<CMySettingsDialog> >(core_api::get_main_window());
	}
	catch (std::exception const & e) {
		popup_message::g_complain("Dialog creation failure", e);
	}
}


LRESULT CMySettingsDialog::OnNMCustomdrawStretchSlider(int /*idCtrl*/, LPNMHDR pNMHDR, BOOL& /*bHandled*/)
{
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	// TODO: Add your control notification handler code here

	return 0;
}

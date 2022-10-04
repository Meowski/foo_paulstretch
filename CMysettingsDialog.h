#pragma once
#ifndef CMYSETTINGSDIALOG_H
#define CMYSETTINGSDIALOG_H


#include "../SDK/foobar2000.h" 
#include "../ATLHelpers/ATLHelpers.h"
#include "paulstretchPreset.h"
#include "resource.h"
#include <libPPUI/CDialogResizeHelper.h>
#include <libPPUI/DarkMode.h>

static const CDialogResizeHelper::Param resizeData[] = {
	// Dialog resize handling matrix, defines how the controls scale with the dialog
	//			 L T R B
	{IDOK,       1,1,1,1 },
	{IDCANCEL,   1,1,1,1 },
	{IDC_STRETCH_SLIDER, 0,0,1,0 },
	{IDC_WINDOW_SIZE_SLIDER,   0,0,1,0 },
	{IDC_WINDOW_SIZE_LABEL,   0,0,0,0 },
	{IDC_STRETCH_LABEL,   0,0,0,0 },
	{IDC_ENABLED_LABEL,   1,1,1,1 },
	{IDC_ENABLE_STRETCH,   1,1,1,1 },

	// current position of a control is determined by initial_position + factor * (current_dialog_size - initial_dialog_size)
	// where factor is the value from the table above
	// applied to all four values - left, top, right, bottom
	// 0,0,0,0 means that a control doesn't react to dialog resizing (aligned to top+left, no resize)
	// 1,1,1,1 means that the control is aligned to bottom+right but doesn't resize
	// 0,0,1,0 means that the control disregards vertical resize (aligned to top) and changes its width with the dialog
};

static const CRect resizeMinMax(100, 100, 1024, 600);

class CMySettingsDialog : public CDialogImpl<CMySettingsDialog>
{
private:
	paulstretchPreset myData;
	std::function<void(paulstretchPreset)> myCallback;

public:

	enum
	{
		IDD = IDD_SETTINGS
	};

<<<<<<< Updated upstream:CMysettingsDialog.h
	CMySettingsDialog(const dsp_preset& paulstretchPresetEntry, std::function<void(paulstretchPreset)> callback)
		: myCallback(callback), myIsModal(true)
=======
	paulstretch_dialog(const dsp_preset& paulstretchPresetEntry, std::function<void(paulstretch_preset)> callback)
		: myCallback(callback), 
		  myIsModal(true),
		  myResizer(resizeData, resizeMinMax),
		  myDarkModeHelper(detectDarkMode())
>>>>>>> Stashed changes:paulstretch_dialog.h
	{
		paulstretchPreset paulstretchPreset;
		paulstretchPreset.readData(paulstretchPresetEntry);
		myData = paulstretchPreset;
	}

<<<<<<< Updated upstream:CMysettingsDialog.h
	CMySettingsDialog()	: myData(), myIsModal(false)
=======
	paulstretch_dialog()
		: myData(),
		myIsModal(false),
		myResizer(resizeData, resizeMinMax),
		myDarkModeHelper(detectDarkMode())
>>>>>>> Stashed changes:paulstretch_dialog.h
	{
		if (!findPaulstretchData())
			pfc::outputDebugLine("Failed to find paulstretch data in 'modeless window' dialog creation.");

		myCallback = [&](paulstretchPreset data) -> void {
			dsp_preset_impl newPresetData;
			myData.writeData(newPresetData);
			fb2k::inMainThread([=]()
			{
				if (!paulstretchPreset::replaceData(newPresetData))
				pfc::outputDebugLine("Failed to replace paulstretch data in 'modeless window' update callback.");	
			});
			
		};

		// If paulstretch is currently defined, we will overwrite the default values above.
		findPaulstretchData();
	}

<<<<<<< Updated upstream:CMysettingsDialog.h
	BEGIN_MSG_MAP(CMySettingsDialog)
=======
	BEGIN_MSG_MAP(paulstretch_dialog)
		CHAIN_MSG_MAP_MEMBER(myResizer)
>>>>>>> Stashed changes:paulstretch_dialog.h
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_HANDLER_EX(IDCANCEL, BN_CLICKED, OnCancel)
		COMMAND_HANDLER_EX(IDC_ENABLE_STRETCH, BN_CLICKED, OnEnabledCheckBoxChanged)
		MSG_WM_HSCROLL(OnHScroll)
		MSG_WM_SIZE(OnSize)
	END_MSG_MAP()

	BOOL OnInitDialog(CWindow, LPARAM)
	{
		// Double border style removes the 
		// icon symbol.
		//
		int extendedStyle = GetWindowLong(-20);
		SetWindowLong(GWL_EXSTYLE, extendedStyle | WS_EX_DLGMODALFRAME);

		myStretchSlider = GetDlgItem(IDC_STRETCH_SLIDER);
		myWindowSizeSlider = GetDlgItem(IDC_WINDOW_SIZE_SLIDER);
		myEnabledCheckBox = GetDlgItem(IDC_ENABLE_STRETCH);

		myStretchSlider.SetRange(ourStretchMin, ourStretchMax);
		myWindowSizeSlider.SetRange(ourWindowSizeMin, ourWindowSizeMax);

		myStretchSlider.SetPos(clamp(ourStretchMin, myData.stretchAmount() * ourStretchMin, ourStretchMax));
		myWindowSizeSlider.SetPos(clamp(ourWindowSizeMin, myData.windowSize() * 1000, ourWindowSizeMax));

		myEnabledCheckBox.SetCheck(myData.enabled());

		updateStretch(myStretchSlider.GetPos());
		updateWindow(myWindowSizeSlider.GetPos());
		checkIfPaulstretchIsEnabled();

		myDarkModeHelper.AddDialogWithControls(this->m_hWnd);
		
		::ShowWindowCentered(static_cast<CWindow>(*this), GetParent());


		return TRUE;
	}

	void OnSize(UINT nType, CSize size) {

	}

	void OnCancel(UINT, int, CWindow) {
		if (myIsModal)
			EndDialog(0);
		else
			DestroyWindow();
	}

	void OnEnabledCheckBoxChanged(UINT, int, CWindow)
	{
		myData.myEnabled = myEnabledCheckBox.GetCheck() == BST_CHECKED ? true : false;
		myCallback(paulstretchPreset(myData.stretchAmount(), myData.windowSize(), myData.enabled()));
	}

	void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar pScrollBar)
	{
		switch (pScrollBar.GetDlgCtrlID())
		{
		case IDC_STRETCH_SLIDER: updateStretch(myStretchSlider.GetPos()); break;
		case IDC_WINDOW_SIZE_SLIDER: updateWindow(myWindowSizeSlider.GetPos()); break;
		default: uBugCheck();
		}

	}

	void updateStretch(int value)
	{
		pfc::string_formatter msg; msg << "Stretch Amount: " << pfc::format_float(value / (1.0f * ourStretchMin), 0, 1);
		uSetDlgItemText(*this, IDC_STRETCH_LABEL, msg);
		myData.myStretchAmount = value / (1.0f * ourStretchMin);
		myCallback(myData);
	}

	void updateWindow(int value)
	{
		pfc::string_formatter msg; msg << "Window Size: " << pfc::format_float(value / 1000.0f, 0, 2) << " s";
		uSetDlgItemText(*this, IDC_WINDOW_SIZE_LABEL, msg);
		myData.myWindowSize = value / 1000.0f;
		myCallback(myData);
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

	int clamp(int minVal, float val, int maxVal)
	{
		return static_cast<int>(min(maxVal, max(minVal, val)));
	}

	int clamp(int minVal, int val, int maxVal)
	{
		return min(maxVal, max(minVal, val));
	}

	bool findPaulstretchData()
	{
		myDSP_manager_ptr = static_api_ptr_t<dsp_config_manager>();
		dsp_preset_impl out;
		if (!myDSP_manager_ptr->core_query_dsp(paulstretchPreset::getGUID(), out))
			return false;
		paulstretchPreset paulstretchPreset;

		if (!paulstretchPreset.readData(out))
			return false;
		myData = paulstretchPreset;

		return true;
	}

	bool detectDarkMode() {
		return true;
	}

	bool isPaulstretchEnabled()
	{
		myDSP_manager_ptr = static_api_ptr_t<dsp_config_manager>();
		dsp_preset_impl out;
		return myDSP_manager_ptr->core_query_dsp(paulstretchPreset::getGUID(), out);
	}

	CTrackBarCtrl myStretchSlider;
	CTrackBarCtrl myWindowSizeSlider;
	CButton myEnabledCheckBox;
	CDialogResizeHelper myResizer;
	DarkMode::CHooks myDarkModeHelper;
	bool myIsModal;
	const static int ourStretchMin = 10;
	const static int ourDefaultStretch = 40;
	const static int ourStretchMax = 400;
	const static int ourWindowSizeMin = 16;
	const static int ourDefaultWindowSize = 280;
	const static int ourWindowSizeMax = 2000;

	static_api_ptr_t<dsp_config_manager> myDSP_manager_ptr;
};

#endif 

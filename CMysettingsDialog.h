#pragma once
#ifndef CMYSETTINGSDIALOG_H
#define CMYSETTINGSDIALOG_H


#include "../SDK/foobar2000.h" 
#include "../ATLHelpers/ATLHelpers.h"
#include "paulstretchPreset.h"
#include "resource.h"

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

	CMySettingsDialog(const dsp_preset& paulstretchPresetEntry, std::function<void(paulstretchPreset)> callback)
		: myCallback(callback), myIsModal(true)
	{
		paulstretchPreset paulstretchPreset;
		paulstretchPreset.readData(paulstretchPresetEntry);
		myData = paulstretchPreset;
	}

	CMySettingsDialog()	: myData(), myIsModal(false)
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

	BEGIN_MSG_MAP(CMySettingsDialog)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_HANDLER_EX(IDCANCEL, BN_CLICKED, OnCancel)
		COMMAND_HANDLER_EX(IDC_ENABLE_STRETCH, BN_CLICKED, OnEnabledCheckBoxChanged)
		MSG_WM_HSCROLL(OnHScroll)
	END_MSG_MAP()

	BOOL OnInitDialog(CWindow, LPARAM)
	{
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

		::ShowWindowCentered(static_cast<CWindow>(*this), GetParent());
		return TRUE;
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

	bool isPaulstretchEnabled()
	{
		myDSP_manager_ptr = static_api_ptr_t<dsp_config_manager>();
		dsp_preset_impl out;
		return myDSP_manager_ptr->core_query_dsp(paulstretchPreset::getGUID(), out);
	}

	CTrackBarCtrl myStretchSlider;
	CTrackBarCtrl myWindowSizeSlider;
	CButton myEnabledCheckBox;
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

#pragma once

#include "enabled_callback.h"
#include <SDK/foobar2000-all.h>
#include <SDK/coreDarkMode.h>
#include <helpers/atl-misc.h>
#include <helpers/WindowPositionUtils.h>
#include <helpers/BumpableElem.h>
#include "paulstretch_preset.h"
#include "paulstretch_menu.h"
#include "resource.h"
#include <cmath>
#include <optional>
#include "layout_types.h"
#include "dialog_wrapper_helpers.h"
#include "dumb_fraction.h"
#include "main.h"

namespace pauldsp {
	static const GUID g_my_ui_element_guid = { 0x8fcd65b5, 0xf1f9, 0x4088, { 0x9d, 0xda, 0xb2, 0xbb, 0x8a, 0xb8, 0x95, 0x9b } };

	class paulstretch_dialog : public CDialogImpl<paulstretch_dialog>
	{
	protected:
		paulstretch_preset myData;
		std::function<void(paulstretch_preset)> myCallback;

		bool myIsUIElement = false;

		CEdit myStretchEdit;
		CButton myStretchApply;
		CEdit myWindowEdit;
		CButton myWindowApply;
		CComboBox myMinStretchCombo;
		CComboBox myMaxStretchCombo;
		CComboBox myMinWindowCombo;
		CComboBox myMaxWindowCombo;
		CComboBox myStretchPrecisionCombo;
		CComboBox myWindowPrecisionCombo;
		CButton myEnabledCheckBox;
		CButton myIsConversionCheckBox;

		clamped_slider myClampedSlider;
		clamped_slider myClampedWindowSlider;

		fb2k::CCoreDarkModeHooks myDarkModeHelper;
		bool myIsModal;

		std::vector<Fraction> myMaxStretchValues;
		std::vector<Fraction> myMinStretchValues;
		std::vector<Fraction> myMaxWindowValues;
		std::vector<Fraction> myMinWindowValues;
		std::vector<Fraction> myStretchPrecisionValues;
		std::vector<Fraction> myWindowPrecisionValues;

		dsp_config_manager::ptr myDspManager;
		std::unique_ptr<unregister_callback, callback_deletor> myDSPChangedCallback;
		HWND myEnabledTooltip;
	public:
		int IDD = IDD_SETTINGS;

		paulstretch_dialog(const dsp_preset& paulstretchpresetentry, std::function<void(paulstretch_preset)> callback)
			: myCallback(callback),
			myIsModal(true),
			myDarkModeHelper(),
			myDspManager(dsp_config_manager::get()),
			myMaxStretchValues({ Fraction(8), Fraction(20), Fraction(40), Fraction(100) }),
			myMinStretchValues({ Fraction(1), Fraction(1, 2) }),
			myMaxWindowValues({ Fraction(1), Fraction(2), Fraction(5) }),
			myMinWindowValues({ Fraction(1, 10), Fraction(1, 100) }),
			myStretchPrecisionValues({ Fraction(1), Fraction(1, 10), Fraction(1, 100), Fraction(1, 1000) }),
			myWindowPrecisionValues({ Fraction(1, 10), Fraction(1, 100), Fraction(1, 1000) })
		{
			paulstretch_preset paulstretchpreset;
			paulstretchpreset.readData(paulstretchpresetentry);
			myData = paulstretchpreset;
		}

		paulstretch_dialog()
			: myData(),
			myIsModal(false),
			myDarkModeHelper(),
			myDspManager(dsp_config_manager::get()),
			myMaxStretchValues({ Fraction(8), Fraction(20), Fraction(40), Fraction(100) }),
			myMinStretchValues({ Fraction(1), Fraction(1, 2) }),
			myMaxWindowValues({ Fraction(1), Fraction(2), Fraction(5) }),
			myMinWindowValues({ Fraction(1, 10), Fraction(1, 100) }),
			myStretchPrecisionValues({ Fraction(1), Fraction(1, 10), Fraction(1, 100), Fraction(1, 1000) }),
			myWindowPrecisionValues({ Fraction(1, 10), Fraction(1, 100), Fraction(1, 1000) })
		{
			if (!findPaulstretchData())
				pfc::outputDebugLine("Failed to find paulstretch data in 'modeless window' dialog creation.");


			myCallback = [](paulstretch_preset data) -> void {
				dsp_preset_impl newPresetData;
				data.writeData(newPresetData);
				fb2k::inMainThread([=]() {
					typedef paulstretch_preset::REPLACE_RESULT replace_result;
					replace_result result = paulstretch_preset::replaceData(newPresetData);
					if (result == replace_result::NOT_FOUND)
						pfc::outputDebugLine("Failed to replace paulstretch data in 'modeless window' update callback: Not Found.");
					});

			};

			// If paulstretch is currently defined, we will overwrite the default values above.
			findPaulstretchData();
		}

		BEGIN_MSG_MAP(paulstretch_dialog)
			MSG_WM_INITDIALOG(OnInitDialog)
			COMMAND_HANDLER_EX(IDCANCEL, BN_CLICKED, OnCancel)
			COMMAND_HANDLER_EX(IDC_ENABLE_STRETCH, BN_CLICKED, OnEnabledCheckBoxChanged)
			COMMAND_HANDLER_EX(IDC_ENABLE_CONVERSION, BN_CLICKED, OnConversionCheckBoxChanged)
			COMMAND_HANDLER_EX(IDC_BUTTON_APPLY_STRETCH, BN_CLICKED, OnStretchApply)
			COMMAND_HANDLER_EX(IDC_BUTTON_APPLY_WINDOW, BN_CLICKED, OnWindowApply)
			COMMAND_HANDLER_EX(IDC_COMBO_STRETCH_MIN, CBN_SELCHANGE, OnStretchMinSelected)
			COMMAND_HANDLER_EX(IDC_COMBO_STRETCH_MAX, CBN_SELCHANGE, OnStretchMaxSelected)
			COMMAND_HANDLER_EX(IDC_COMBO_WINDOW_MIN, CBN_SELCHANGE, OnWindowMinSelected)
			COMMAND_HANDLER_EX(IDC_COMBO_WINDOW_MAX, CBN_SELCHANGE, OnWindowMaxSelected)
			COMMAND_HANDLER_EX(IDC_COMBO_STRETCH_PRECISION, CBN_SELCHANGE, OnStretchPrecisionSelected)
			COMMAND_HANDLER_EX(IDC_COMBO_WINDOW_PRECISION, CBN_SELCHANGE, OnWindowPrecisionSelected)
			MSG_WM_SIZE(OnSize)
			MSG_WM_HSCROLL(OnHScroll)
			MSG_WM_DESTROY(OnDestroy);
		END_MSG_MAP()

	protected:
		// Need the addresses for the rows, so make them data members.
		// 
		// Row One
		// 		
		StaticTextCell textCell;
		EditCell editCell;
		ButtonCell buttonCell;
		// Two
		ComboCell min_stretch_cell;
		SliderCell stretch_slider_cell;
		ComboCell max_stretch_cell;
		// Three
		StaticTextCell precision_stretch_static_cell;
		ComboCell precision_stretch_combo_cell;
		// Four
		StaticTextCell window_apply_static_cell;
		EditCell window_apply_edit_cell;
		ButtonCell window_apply_button_cell;
		// Five
		ComboCell min_window_cell;
		SliderCell window_slider_cell;
		ComboCell max_window_cell;
		// Six
		StaticTextCell precision_window_static_cell;
		ComboCell precision_window_combo_cell;
		// Seven
		CheckboxCell enabled_checkbox_cell;
		// Either
		CheckboxCell conversion_checkbox_cell;

		Column myColumn;

	protected:
	
		void changeCallback() {
			if (!core_api::is_main_thread())
				return;
			
			dsp_preset_impl preset;
			myDspManager->core_query_dsp(paulstretch_preset::getGUID(), preset);
			paulstretch_preset newPreset{ preset };
			if (newPreset.enabled() == myData.enabled())
				return;

			myData.myEnabled = newPreset.enabled();
			myEnabledCheckBox.SetCheck(newPreset.enabled());
		}

		void initLayout()
		{
			Padding padding(2, 3, 2, 3);
			std::vector<std::vector<ICell*>> rows;
			for (size_t i = 0; i <= 7; ++i)
				rows.push_back(std::vector<ICell*>());

			int currentRow = 0;
			// Row one
			//
			CStatic stretch_label = GetDlgItem(IDC_STRETCH_LABEL);
			CEdit stretch_edit = GetDlgItem(IDC_EDIT_STRETCH);
			CButton stretch_apply = GetDlgItem(IDC_BUTTON_APPLY_STRETCH);
			textCell = StaticTextCell(stretch_label, padding);
			editCell = EditCell(stretch_edit, 7, padding);
			buttonCell = ButtonCell(stretch_apply, Padding(2, 5, 2, 5));
			rows[currentRow].push_back(&textCell);
			rows[currentRow].push_back(&editCell);
			rows[currentRow].push_back(&buttonCell);

			currentRow++;
			// Row two
			//
			CComboBox min_stretch_combo = GetDlgItem(IDC_COMBO_STRETCH_MIN);
			CTrackBarCtrl stretch_slider = GetDlgItem(IDC_STRETCH_SLIDER);
			CComboBox max_stretch_combo = GetDlgItem(IDC_COMBO_STRETCH_MAX);
			min_stretch_cell = ComboCell(min_stretch_combo, padding);
			stretch_slider_cell = SliderCell(stretch_slider, padding);
			stretch_slider_cell.setFlex(1);
			max_stretch_cell = ComboCell(max_stretch_combo, padding);
			rows[currentRow].push_back(&min_stretch_cell);
			rows[currentRow].push_back(&stretch_slider_cell);
			rows[currentRow].push_back(&max_stretch_cell);

			currentRow++;
			// Row Three
			//
			CStatic stretch_precision_label = GetDlgItem(IDC_STATIC_STRETCH_PRECISION);
			CComboBox stretch_precision_combo = GetDlgItem(IDC_COMBO_STRETCH_PRECISION);
			precision_stretch_static_cell = StaticTextCell(stretch_precision_label, padding);
			precision_stretch_combo_cell = ComboCell(stretch_precision_combo, padding);
			rows[currentRow].push_back(&precision_stretch_static_cell);
			rows[currentRow].push_back(&precision_stretch_combo_cell);

			currentRow++;
			// Row Four
			//
			CStatic window_label = GetDlgItem(IDC_WINDOW_SIZE_LABEL);
			CEdit window_edit = GetDlgItem(IDC_EDIT_WINDOW);
			CButton window_apply = GetDlgItem(IDC_BUTTON_APPLY_WINDOW);
			window_apply_static_cell = StaticTextCell(window_label, padding);
			window_apply_edit_cell = EditCell(window_edit, 7, padding);
			window_apply_button_cell = ButtonCell(window_apply, Padding(2, 5, 2, 5));
			rows[currentRow].push_back(&window_apply_static_cell);
			rows[currentRow].push_back(&window_apply_edit_cell);
			rows[currentRow].push_back(&window_apply_button_cell);

			currentRow++;
			// Row Five
			//
			CComboBox min_window_combo = GetDlgItem(IDC_COMBO_WINDOW_MIN);
			CTrackBarCtrl window_slider = GetDlgItem(IDC_WINDOW_SIZE_SLIDER);
			CComboBox max_window_combo = GetDlgItem(IDC_COMBO_WINDOW_MAX);
			min_window_cell = ComboCell(min_window_combo, padding);
			window_slider_cell = SliderCell(window_slider, padding);
			window_slider_cell.setFlex(1);
			max_window_cell = ComboCell(max_window_combo, padding);
			rows[currentRow].push_back(&min_window_cell);
			rows[currentRow].push_back(&window_slider_cell);
			rows[currentRow].push_back(&max_window_cell);

			currentRow++;
			// Row Six
			CStatic slider_precision_static = GetDlgItem(IDC_STATIC_WINDOW_PRECISION);
			CComboBox slider_precision_combo = GetDlgItem(IDC_COMBO_WINDOW_PRECISION);
			precision_window_static_cell = StaticTextCell(slider_precision_static, padding);
			precision_window_combo_cell = ComboCell(slider_precision_combo, padding);
			rows[currentRow].push_back(&precision_window_static_cell);
			rows[currentRow].push_back(&precision_window_combo_cell);

			currentRow++;
			// Row Seven
			CCheckBox enabled_checkbox = GetDlgItem(IDC_ENABLE_STRETCH);
			enabled_checkbox_cell = CheckboxCell(enabled_checkbox, padding);
			rows[currentRow].push_back(&enabled_checkbox_cell);

			currentRow++;
			//RowEight
			CCheckBox conversion_checkbox = GetDlgItem(IDC_ENABLE_CONVERSION);
			conversion_checkbox_cell = CheckboxCell(conversion_checkbox, padding);
			rows[currentRow].push_back(&conversion_checkbox_cell);


			Margin margin(5, 5, 5, 5);
			Margin closerMargin(5, 1, 5, 1);
			myColumn = Column(std::vector<Row>{
				Row(rows[0], 5, CENTER, margin, 1),
					Row(rows[1], 5, CENTER, margin),
					Row(rows[2], 5, RIGHT, closerMargin),
					Row(rows[3], 5, CENTER, margin, 1),
					Row(rows[4], 5, CENTER, margin),
					Row(rows[5], 5, RIGHT, closerMargin),
					Row(rows[6], 5, LEFT, closerMargin),
					Row(rows[7], 5, LEFT, closerMargin)
			});
		}

		virtual BOOL OnSize(UINT, CSize)
		{	
			RECT rect;
			GetClientRect(&rect);
			CPaintDC dc(*this);
			HDWP hdwp = BeginDeferWindowPos(16);
			auto [area, returnedHDWP] = myColumn.layout(hdwp, &dc, Region(rect), paulstretch_dialog::m_hWnd);
			if (returnedHDWP != NULL)
				EndDeferWindowPos(returnedHDWP);

			return true;
		}

		virtual void OnInit() {}

		BOOL OnInitDialog(CWindow, LPARAM)
		{
			initLayout();
			OnSize(UINT(), CSize());
			OnInit();
			
			if (myIsUIElement)
				myDSPChangedCallback = get_enabled_callback().registerCallback([&]() -> void { changeCallback(); });

			if (!myIsUIElement)
			{
				// Double border style removes the icon symbol for a modal dialog box.
				//
				int extendedStyle = GetWindowLong(GWL_EXSTYLE);
				SetWindowLong(GWL_EXSTYLE, extendedStyle | WS_EX_DLGMODALFRAME);
			}

			myMaxStretchCombo = GetDlgItem(IDC_COMBO_STRETCH_MAX);
			myMinStretchCombo = GetDlgItem(IDC_COMBO_STRETCH_MIN);
			myMaxWindowCombo = GetDlgItem(IDC_COMBO_WINDOW_MAX);
			myMinWindowCombo = GetDlgItem(IDC_COMBO_WINDOW_MIN);
			myStretchPrecisionCombo = GetDlgItem(IDC_COMBO_STRETCH_PRECISION);
			myWindowPrecisionCombo = GetDlgItem(IDC_COMBO_WINDOW_PRECISION);

			myStretchEdit = GetDlgItem(IDC_EDIT_STRETCH);
			myStretchEdit.SetLimitText(15);
			myWindowEdit = GetDlgItem(IDC_EDIT_WINDOW);
			myWindowEdit.SetLimitText(15);
			myEnabledCheckBox = GetDlgItem(IDC_ENABLE_STRETCH);
			myIsConversionCheckBox = GetDlgItem(IDC_ENABLE_CONVERSION);

			selection_handler minStretchSelector(myMinStretchCombo, myMinStretchValues, Fraction(1));
			selection_handler maxStretchSelector(myMaxStretchCombo, myMaxStretchValues, Fraction(8));
			selection_handler precisionStretchSelector(myStretchPrecisionCombo, myStretchPrecisionValues, Fraction(1, 10));
			CTrackBarCtrl slider = GetDlgItem(IDC_STRETCH_SLIDER);
			myClampedSlider = clamped_slider(minStretchSelector, slider, myStretchEdit, maxStretchSelector, precisionStretchSelector);
			myClampedSlider.init(minStretch(), stretchAmount(), maxStretch(), stretchPrecision());

			selection_handler minWindowSelector(myMinWindowCombo, myMinWindowValues, Fraction(1, 100));
			selection_handler maxWindowSelector(myMaxWindowCombo, myMaxWindowValues, Fraction(2));
			selection_handler precisionWindowSelector(myWindowPrecisionCombo, myWindowPrecisionValues, Fraction(1, 100));
			CTrackBarCtrl windowSlider = GetDlgItem(IDC_WINDOW_SIZE_SLIDER);
			myClampedWindowSlider = clamped_slider(minWindowSelector, windowSlider, myWindowEdit, maxWindowSelector, precisionWindowSelector);
			myClampedWindowSlider.init(minWindow(), windowSize(), maxWindow(), windowPrecision());

			myEnabledCheckBox.SetCheck(myData.enabled());
			myIsConversionCheckBox.SetCheck(myData.isConversion());

			myDarkModeHelper.AddDialogWithControls(this->m_hWnd);
			//myEnabledTooltip = CreateToolTip(IDC_ENABLE_STRETCH, this->m_hWnd, L"Example tooltip.");

			//if (!myIsUIElement)
			::ShowWindowCentered(static_cast<CWindow>(*this), GetParent());
			myStretchEdit.SetSelNone();

			return FALSE;
		}

		// Description:
	//   Creates a tooltip for an item in a dialog box. 
	// Parameters:
	//   idTool - identifier of an dialog box item.
	//   nDlg - window handle of the dialog box.
	//   pszText - string to use as the tooltip text.
	// Returns:
	//   The handle to the tooltip.
	//
		/*
		HWND CreateToolTip(int toolID, HWND hDlg, PTSTR pszText)
		{
			if (!toolID || !hDlg || !pszText)
			{
				return FALSE;
			}
			// Get the window of the tool.
			HWND hwndTool = GetDlgItem(toolID);

			// Create the tooltip. g_hInst is the global instance handle.
			HWND hwndTip = CreateWindowEx(NULL, TOOLTIPS_CLASS, NULL,
				WS_POPUP | TTS_ALWAYSTIP,
				CW_USEDEFAULT, CW_USEDEFAULT,
				CW_USEDEFAULT, CW_USEDEFAULT,
				hDlg, NULL,
				core_api::get_my_instance(), NULL);

			if (!hwndTool || !hwndTip)
			{
				return (HWND)NULL;
			}

			// Associate the tooltip with the tool.
			TOOLINFO toolInfo = { 0 };
			toolInfo.cbSize = sizeof(toolInfo);
			toolInfo.hwnd = hDlg;
			toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
			toolInfo.uId = (UINT_PTR)hwndTool;
			toolInfo.lpszText = pszText;
			SendMessage(hwndTip, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);
			SendMessage(hwndTip, TTM_SETMAXTIPWIDTH, 0, 500);

			return hwndTip;
		}
		*/

		void OnDestroy() {
			if (myEnabledTooltip != NULL)
				::DestroyWindow(myEnabledTooltip);
			SetMsgHandled(false);
		}

		void OnCancel(UINT, int, CWindow) {
			if (myIsModal)
				EndDialog(0);
			else
				DestroyWindow();
		}

		double onEditApply(CEdit editControl, clamped_slider clampedSlider, double oldValue)
		{
			CString text;
			wchar_t* endChar;
			editControl.GetWindowTextW(text);
			double newAmount = wcstod(text, &endChar);
			if (newAmount == 0.0 || newAmount == HUGE_VAL || newAmount == -HUGE_VAL)
			{
				// Use the scroll event to reset to current value
				clampedSlider.onScroll();
				return oldValue;
			}

			return clampedSlider.onEditApply(newAmount);
		}

		void OnStretchApply(UINT, int, CWindow)
		{
			double newStretch = onEditApply(myStretchEdit, myClampedSlider, stretchAmount());
			updateStretch(newStretch);
			myCallback(myData);
		}

		void OnWindowApply(UINT, int, CWindow)
		{
			double newWindow = onEditApply(myWindowEdit, myClampedWindowSlider, windowSize());
			updateWindow(newWindow);
			myCallback(myData);
		}

		void OnStretchMaxSelected(UINT, int, CWindow)
		{
			Fraction newMaxStretch = myClampedSlider.onMaxChanged();
			updateStretch(myClampedSlider.getValueAsDouble());
			updateMaxStretch(newMaxStretch);
			myCallback(myData);
		}

		void OnWindowMaxSelected(UINT, int, CWindow)
		{
			Fraction newMaxWindow = myClampedWindowSlider.onMaxChanged();
			updateWindow(myClampedWindowSlider.getValueAsDouble());
			updateMaxWindow(newMaxWindow);
			myCallback(myData);
		}

		void OnWindowMinSelected(UINT, int, CWindow)
		{
			Fraction newMin = myClampedWindowSlider.onMinChanged();
			updateWindow(myClampedWindowSlider.getValueAsDouble());
			updateMinWindow(newMin);
			myCallback(myData);
		}

		void OnStretchMinSelected(UINT, int, CWindow)
		{
			Fraction newMin = myClampedSlider.onMinChanged();
			updateStretch(myClampedSlider.getValueAsDouble());
			updateMinStretch(newMin);
			myCallback(myData);
		}

		void OnStretchPrecisionSelected(UINT, int, CWindow)
		{
			Fraction value = myClampedSlider.onPrecisionChanged();
			updateStretchPrecision(value);
			updateStretch(myClampedSlider.getValueAsDouble());
			myCallback(myData);
		}

		void OnWindowPrecisionSelected(UINT, int, CWindow)
		{
			Fraction value = myClampedWindowSlider.onPrecisionChanged();
			updateWindowPrecision(value);
			updateWindow(myClampedWindowSlider.getValueAsDouble());
			myCallback(myData);
		}

		void updateMaxStretch(Fraction newMaxStretch)
		{
			if (myData.myMaxStretch == newMaxStretch)
				return;

			myData.myMaxStretch = newMaxStretch;
		}

		void updateMaxWindow(Fraction newMaxWindow)
		{
			if (myData.myMaxWindow == newMaxWindow)
				return;

			myData.myMaxWindow = newMaxWindow;
		}

		void updateMinWindow(Fraction newMinWindow)
		{
			if (myData.myMinWindow == newMinWindow)
				return;

			myData.myMinWindow = newMinWindow;
		}

		void updateMinStretch(Fraction newMinStretch)
		{
			if (myData.myMinStretch == newMinStretch)
				return;

			myData.myMinStretch = newMinStretch;
		}

		void OnEnabledCheckBoxChanged(UINT, int, CWindow)
		{
			myData.myEnabled = myEnabledCheckBox.GetCheck() == BST_CHECKED ? true : false;
			if (!isPaulstretchEnabled() && myData.myEnabled)
			{
				dsp_preset_impl presetToActivate;
				myData.writeData(presetToActivate);
				myDspManager->core_enable_dsp(presetToActivate, dsp_config_manager::default_insert_last);
				return;
			}
			myCallback(myData);
		}

		void OnConversionCheckBoxChanged(UINT, int, CWindow)
		{
			myData.myIsConversion = myIsConversionCheckBox.GetCheck() == BST_CHECKED ? true : false;
			myCallback(myData);
		}

		void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar pScrollBar)
		{
			Fraction stretchValue = myClampedSlider.onScroll();
			Fraction windowValue = myClampedWindowSlider.onScroll();

			// only actually push the settings to the DSP after we release the scroll bar
			if (nSBCode != SB_THUMBPOSITION && nSBCode != SB_ENDSCROLL)
				return;

			updateStretch(myClampedSlider.getValueAsDouble());
			updateWindow(myClampedWindowSlider.getValueAsDouble());
			myCallback(myData);
		}

		void updateStretchPrecision(Fraction value)
		{
			if (value == myData.myStretchPrecision)
				return;

			myData.myStretchPrecision = value;
		}

		void updateWindowPrecision(Fraction value)
		{
			if (value == myData.myWindowPrecision)
				return;

			myData.myWindowPrecision = value;
		}

		void updateStretch(double value)
		{
			if (value == myData.myStretchAmount)
				return;

			myData.myStretchAmount = value;
		}

		void updateWindow(double value)
		{
			if (value == myData.myWindowSize)
				return;

			myData.myWindowSize = value;
		}

		dsp_preset_impl preset()
		{
			dsp_preset_impl data;
			myData.writeData(data);
			return data;
		}

	private:

		Fraction clamp(Fraction minVal, double val, Fraction maxVal)
		{
			if (minVal.getNumerator() >= val * minVal.getDenominator())
				return minVal;
			else if (maxVal.getNumerator() <= val * maxVal.getDenominator())
				return maxVal;
			return Fraction::fromDouble(val);
		}

		int clamp(int minVal, double val, int maxVal)
		{
			int roundedVal = (int)round(val);
			return clamp(minVal, roundedVal, maxVal);
		}

		int clamp(int minVal, int val, int maxVal)
		{
			return min(maxVal, max(minVal, val));
		}

		bool findPaulstretchData()
		{
			dsp_preset_impl out;
			if (!myDspManager->core_query_dsp(paulstretch_preset::getGUID(), out))
				return false;
			paulstretch_preset paulstretchPreset;

			if (!paulstretchPreset.readData(out))
				return false;
			myData = paulstretchPreset;

			return true;
		}

		//bool detectDarkMode() {
		//	return fb2k::isDarkMode();
		//}

		bool isPaulstretchEnabled()
		{
			dsp_preset_impl out;
			return myDspManager->core_query_dsp(paulstretch_preset::getGUID(), out);
		}

		double windowSize() {
			return myData.windowSize();
		}

		double stretchAmount() {
			return myData.stretchAmount();
		}

		Fraction maxStretch() {
			return myData.myMaxStretch;
		}

		Fraction minStretch() {
			return myData.myMinStretch;
		}

		Fraction minWindow() {
			return myData.myMinWindow;
		}

		Fraction maxWindow() {
			return myData.myMaxWindow;
		}

		Fraction stretchPrecision() {
			return myData.myStretchPrecision;
		}

		Fraction windowPrecision() {
			return myData.myWindowPrecision;
		}
	};


	class paulstretch_ui_element : public paulstretch_dialog, public ui_element_instance
	{
	public:

		const ui_element_instance_callback::ptr m_callback;

		paulstretch_ui_element(ui_element_config::ptr cfg, ui_element_instance_callback::ptr cb)
			: m_callback(cb)
		{
			myIsUIElement = true;
			paulstretch_dialog::IDD = IDD_SETTINGS1;
		}

		BOOL OnSize(UINT, CSize) override
		{
			RECT rect;
			GetClientRect(&rect);
			CPaintDC dc(*this);
			SelectObjectScope scope(dc, (HGDIOBJ)m_callback->query_font_ex(ui_font_default));
			HDWP hdwp = BeginDeferWindowPos(16);
			auto [area, returnedHDWP] = myColumn.layout(hdwp, &dc, Region(rect), paulstretch_dialog::m_hWnd);
			if (returnedHDWP != NULL)
				EndDeferWindowPos(returnedHDWP);

			return false;
		}

		void OnInit() override
		{
			auto theFont = m_callback->query_font_ex(ui_font_default);
			EnumChildWindows(
				m_hWnd,
				[&](HWND wnd) -> void {
					CWindow thatWindow(wnd);
					thatWindow.SetFont(theFont, false);
					return;
				}
			);
		}

		void initialize_window(HWND parent) { WIN32_OP(Create(parent) != NULL); }
		HWND get_wnd() { return m_hWnd; }
		void set_configuration(ui_element_config::ptr config) {
			m_callback->on_min_max_info_change();
		}
		ui_element_config::ptr get_configuration() { return g_get_default_configuration(); }
		static GUID g_get_guid() {
			return g_my_ui_element_guid;
		}
		static void g_get_name(pfc::string_base& out) { out = "Paulstretch"; }
		static ui_element_config::ptr g_get_default_configuration() {
			return ui_element_config::g_create_empty(g_my_ui_element_guid);
		}
		static const char* g_get_description() { return "Paulstretch Settings"; }
		static GUID g_get_subclass() {
			return ui_element_subclass_dsp;
		}

		ui_element_min_max_info get_min_max_info() {
			ui_element_min_max_info ret;

			// Note that we play nicely with separate horizontal & vertical DPI.
			// Such configurations have not been ever seen in circulation, but nothing stops us from supporting such.
			CSize DPI = QueryScreenDPIEx(*this);

			if (DPI.cx <= 0 || DPI.cy <= 0) { // sanity
				DPI = CSize(96, 96);
			}

			// Just don't want it vanishing to nothing, but I otherwise
			// don't care if controls are clipped.
			ret.m_min_width = MulDiv(50, DPI.cx, 96);
			ret.m_min_height = MulDiv(50, DPI.cy, 96);

			// Deal with WS_EX_STATICEDGE and alike that we might have picked from host
			ret.adjustForWindow(*this);

			return ret;
		}

		void notify(const GUID& p_what, t_size p_param1, const void* p_param2, t_size p_param2size) override {
			if (p_what == ui_element_notify_colors_changed || p_what == ui_element_notify_font_changed)
			{
				EnumChildWindows(
					m_hWnd,
					[=](HWND wnd) -> void {
						CWindow thatWindow(wnd);
						thatWindow.SetFont(m_callback->query_font_ex(ui_font_default), false);
						return;
					}
				);
				OnSize(UINT(), CSize());
			}
		}
	};

	class ui_popup_wrapper : public ui_element_impl_withpopup<paulstretch_ui_element>
	{
		bool get_menu_command_description(pfc::string_base& out)
		{
			out = "Paulstretch Settings";
			return true;
		}
	};
}
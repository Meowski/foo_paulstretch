#pragma once

#include <atlapp.h>
#include <windef.h>

namespace pauldsp {

	class Padding
	{
	public:
		int left;
		int top;
		int right;
		int bottom;

		Padding(int left, int top, int right, int bottom) : left(left), top(top), right(right), bottom(bottom) {}

		Padding() : Padding(0, 0, 0, 0) {}

		int leftRight()
		{
			return left + right;
		}

		int topBottom()
		{
			return top + bottom;
		}
	};

	class Margin : public Padding
	{
	public:

		Margin(int left, int top, int right, int bottom) : Padding(left, top, right, bottom) {}

		Margin() : Padding(0, 0, 0, 0) {}
	};

	enum Justification
	{
		LEFT,
		CENTER,
		RIGHT
	};

	class Point
	{
	public:
		int x;
		int y;

		Point(int x = 0, int y = 0) : x(x), y(y) {}
	};

	class Area
	{
	public:
		int width;
		int height;

		Area(int width = 0, int height = 0) : width(width), height(height) {}
	};

	class Region
	{
	public:
		int width;
		int height;
		Point origin;

		Region(RECT r) :
			width(r.right - r.left),
			height(r.bottom - r.top),
			origin(r.left, r.top)
		{}

		Region(Point origin, int width, int height) : origin(origin), width(width), height(height) {}
		Region(int width, int height) : Region(Point(), width, height) {}

		Area area()
		{
			return Area(width, height);
		}
	};

	class Step
	{
	public:
		int x;
		int y;

		Step(int x, int y) : x(x), y(y) {}
	};

	class ICell {
	protected:
		int myFlex;
		Padding myPadding;
		Margin myMargin;

		ICell() : myPadding(Padding()), myMargin(Margin()), myFlex(0) {}
		ICell(Padding padding) : myPadding(padding), myMargin(Margin()), myFlex(0) {}
		ICell(Margin margin) : myPadding(Padding()), myMargin(margin), myFlex(0) {}
		ICell(int flex) : myPadding(Padding()), myMargin(Margin()), myFlex(flex) {}
		ICell(Padding padding, Margin margin) : myPadding(padding), myMargin(margin), myFlex(0) {}
		ICell(Margin margin, int flex) : myPadding(Padding()), myMargin(margin), myFlex(flex) {}
		ICell(Padding padding, int flex) : myPadding(padding), myMargin(Margin()), myFlex(flex) {}
		ICell(Padding padding, Margin margin, int flex) : myPadding(padding), myMargin(margin), myFlex(flex) {}

	public:
		virtual Area getDesiredArea(const CPaintDC* dc, HWND dialog_hwnd) = 0;
		virtual HWND getHWND() = 0;
		virtual HDWP move(HDWP hdwp, int x, int y, int width, int height)
		{
			if (hdwp == NULL)
				return NULL;

			auto FLAGS = SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_NOCOPYBITS;
			return DeferWindowPos(hdwp, getHWND(), NULL, x, y, width, height, FLAGS);
		}

		int getFlex()
		{
			return myFlex;
		}

		void setFlex(int flex)
		{
			myFlex = flex;
		}
	};

	class StaticTextCell : public ICell {
	public:
		CStatic myText;

		StaticTextCell(CStatic text = CStatic()) : StaticTextCell(text, Padding()) {}

		StaticTextCell(CStatic text, Padding padding) : myText(text), ICell(padding) {}

		Area getDesiredArea(const CPaintDC* dc, HWND dialog_hwnd) override
		{
			CString out;
			myText.GetWindowText(out);
			RECT s{ 0, 0, 0, 0 };
			DrawText(dc->m_hDC, out, out.GetLength(), &s, DT_CALCRECT);

			return Area(s.right + myPadding.leftRight(), s.bottom + myPadding.topBottom());
		}

		HWND getHWND() override
		{
			return myText.m_hWnd;
		}
	};

	class ComboCell : public ICell
	{
	public:
		CComboBox myCombo;

		ComboCell(CComboBox combo = CComboBox(), Padding padding = Padding()) : myCombo(combo), ICell(padding) {}

		Area getDesiredArea(const CPaintDC* dc, HWND dialog_hwnd) override
		{
			CString out;
			out.Append(L"0.001XXX");

			RECT s{ 0, 0, 0, 0 };
			DrawText(dc->m_hDC, out, out.GetLength(), &s, DT_CALCRECT);

			return Area(s.right + myPadding.leftRight(), s.bottom + myPadding.topBottom());
		}

		HWND getHWND() override
		{
			return myCombo.m_hWnd;
		}
	};

	class SliderCell : public ICell
	{
	public:
		CTrackBarCtrl mySlider;

		SliderCell(CTrackBarCtrl slider = CTrackBarCtrl(), Padding padding = Padding(), int flex = 0) : mySlider(slider), ICell(padding, flex) {}

		Area getDesiredArea(const CPaintDC* dc, HWND dialog_hwnd) override
		{
			CString out;
			out.Append(L"0000000000");

			RECT s{ 0, 0, 0, 0 };
			DrawText(dc->m_hDC, out, out.GetLength(), &s, DT_CALCRECT);

			RECT r;
			GetClientRect(mySlider, &r);

			return Area(s.right + myPadding.leftRight(), s.bottom + myPadding.topBottom());
		}

		HWND getHWND() override
		{
			return mySlider.m_hWnd;
		}

		HDWP move(HDWP hdwp, int x, int y, int width, int height) override
		{
			int scaling = height;
			mySlider.SetThumbLength(max(2, scaling));
			return ICell::move(hdwp, x, y - 1, width, height);
		}
	};

	class EditCell : public ICell {
	public:
		CEdit myEdit;
		size_t myMaxDesiredText;

		EditCell(CEdit edit = CEdit(), size_t maxDesiredText = 5) : EditCell(edit, maxDesiredText, Padding()) {}

		EditCell(CEdit edit, size_t maxDesiredText, Padding padding) : myEdit(edit), myMaxDesiredText(maxDesiredText), ICell(padding) {}

		Area getDesiredArea(const CPaintDC* dc, HWND dialog_hwnd) override
		{
			CString out;
			size_t count = myMaxDesiredText + 1;
			while (count-- > 0)
				out.AppendChar(L'0');

			RECT s{ 0, 0, 0, 0 };
			DrawText(dc->m_hDC, out, out.GetLength(), &s, DT_CALCRECT);

			return Area(s.right + myPadding.leftRight(), s.bottom + myPadding.topBottom());
		}

		HWND getHWND() override
		{
			return myEdit.m_hWnd;
		}
	};

	class ButtonCell : public ICell {
	public:
		CButton myButton;
		Padding myPadding;

		ButtonCell(CButton button = CButton()) : ButtonCell(button, Padding()) {}

		ButtonCell(CButton button, Padding padding) : myButton(button), myPadding(padding) {}

		Area getDesiredArea(const CPaintDC* dc, HWND dialog_hwnd) override
		{
			CString out;
			myButton.GetWindowText(out);
			out.Append(L"0000");

			RECT s{ 0, 0, 0, 0 };
			DrawText(dc->m_hDC, out, out.GetLength(), &s, DT_CALCRECT);

			return Area(myPadding.leftRight() + s.right, myPadding.topBottom() + s.bottom);
		}

		HWND getHWND() override
		{
			return myButton.m_hWnd;
		}
	};

	class CheckboxCell : public ICell {
	public:
		CCheckBox myCheckbox;
		Padding myPadding;

		CheckboxCell(CCheckBox checkBox = CCheckBox()) : CheckboxCell(checkBox, Padding()) {}

		CheckboxCell(CCheckBox checkBox, Padding padding) : myCheckbox(checkBox), myPadding(padding) {}

		Area getDesiredArea(const CPaintDC* dc, HWND dialog_hwnd) override
		{
			CString out;
			myCheckbox.GetWindowText(out);
			out.Append(L"000");

			RECT s{ 0, 0, 0, 0 };
			DrawText(dc->m_hDC, out, out.GetLength(), &s, DT_CALCRECT);

			return Area(myPadding.leftRight() + s.right, myPadding.topBottom() + s.bottom);
		}

		HWND getHWND() override
		{
			return myCheckbox.m_hWnd;
		}
	};

	class Row
	{
	public:
		Justification myJustification;
		Margin myMargin;
		int myGap;
		int myCenterOnIndex;
		std::vector<ICell*> myCells;

		Row() : Row(std::vector<ICell*>()) {}
		Row(std::vector<ICell*> cells) : Row(cells, int()) {}
		Row(std::vector<ICell*> cells, int gap) : Row(cells, gap, CENTER) {}
		Row(std::vector<ICell*> cells, int gap, Justification justification)
			: Row(cells, gap, justification, Margin()) {}
		Row(std::vector<ICell*> cells, int gap, Justification justification, Margin margin)
			: Row(cells, gap, justification, margin, -1) {}
		Row(std::vector<ICell*> cells, int gap, Justification justification, Margin margin, int centerOnIndex)
			: myCells(cells), myJustification(justification), myGap(gap), myMargin(margin), myCenterOnIndex(centerOnIndex) {}

		std::tuple<Area, HDWP> layout(HDWP hdwp, const CPaintDC* dc, Region parentRect, HWND dialog_hwnd)
		{
			// Start with a 0,0 coordinate system
			Point offset = parentRect.origin;
			parentRect.origin = Point(0, 0);
			parentRect.width -= offset.x;
			parentRect.height -= offset.y;

			if (myCells.empty())
				return std::make_tuple<Area, HDWP>(Area(0, 0), NULL);

			int maxHeight = 0;
			int width = 0;
			int totalFlex = 0;

			for (ICell* cell : myCells)
			{
				Area r = cell->getDesiredArea(dc, dialog_hwnd);
				maxHeight = max(maxHeight, r.height);
				width += r.width;
				totalFlex += cell->getFlex();
			}
			width += (static_cast<int>(myCells.size()) - 1) * myGap;
			width += myMargin.leftRight();

			int x = parentRect.origin.x;
			int y = parentRect.origin.y;


			int leftOverWidth = max(0, parentRect.width - width);
			// Justification only makes sense if no one is flexing.
			//
			if (totalFlex <= 0)
			{
				// As a special case of justification, center on a particular cell.
				//
				if (0 <= myCenterOnIndex && myCenterOnIndex < static_cast<int>(myCells.size()))
				{
					int widthTilElement = 0;
					for (int i = 0; i < myCenterOnIndex; ++i)
						widthTilElement += myCells[i]->getDesiredArea(dc, dialog_hwnd).width;
					widthTilElement += myCells[myCenterOnIndex]->getDesiredArea(dc, dialog_hwnd).width / 2;
					x = (parentRect.width / 2) - widthTilElement;
				}
				else
				{
					switch (myJustification)
					{
					case CENTER: x = (parentRect.width + (-width)) / 2; break;
					case LEFT: break;
					case RIGHT: x = parentRect.width - width; break;
					}
				}
			}
			else
				width += leftOverWidth;

			// Don't forget offset now that we're actually drawing!
			//
			x += (myMargin.left + offset.x);
			y += (myMargin.top + offset.y);

			for (ICell* cell : myCells)
			{
				Area r = cell->getDesiredArea(dc, dialog_hwnd);
				int leftoverHeight = max(0, maxHeight - r.height);
				if (totalFlex > 0)
				{
					int adjustment = static_cast<int>(floor(leftOverWidth * double(cell->getFlex()) / double(totalFlex)));
					hdwp = cell->move(hdwp, x, y, r.width + adjustment, maxHeight);
					x += r.width + myGap + adjustment;
				}
				else
				{
					int verticalAdjustment = (maxHeight - r.height) / 2;
					hdwp = cell->move(hdwp, x, y + verticalAdjustment, r.width, r.height);
					x += r.width + myGap;
				}
			}

			return std::make_tuple(Area(width + myMargin.leftRight(), maxHeight + myMargin.topBottom()), hdwp);
		}
	};

	class Column
	{
	public:
		std::vector<Row> myRows;

		Column() : Column(std::vector<Row>()) {}
		Column(std::vector<Row> rows) : myRows(rows) {}

		// Returns the area used by the control.
		//
		std::tuple<Area, HDWP> layout(HDWP hdwp, const CPaintDC* dc, Region parentRect, HWND dialog_hwnd)
		{
			Region currentRect = parentRect;
			Area spaceUsed{ 0,0 };
			for (Row& row : myRows)
			{
				if (hdwp == NULL)
					return std::make_tuple<Area, HDWP>(Area(0, 0), NULL);
				auto [result, hdwp_returned] = row.layout(hdwp, dc, currentRect, dialog_hwnd);
				hdwp = hdwp_returned;
				spaceUsed.width = max(spaceUsed.width, result.width);
				spaceUsed.height += result.height;
				currentRect.origin.y += result.height;
				currentRect.height -= result.height;
			}

			return std::make_tuple(spaceUsed, hdwp);
		}
	};
}
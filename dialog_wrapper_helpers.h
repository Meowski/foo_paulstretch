#pragma once
#include <vector>
#include <atlctrls.h>
#include "dumb_fraction.h"

namespace pauldsp {

	class selection_handler
	{
	private:
		std::vector<Fraction> myValues;
		CComboBox myComboBox;
		Fraction myDefault;

		bool outOfBounds(int index)
		{
			return index < 0 || index >= static_cast<int>(myValues.size());
		}

		void init()
		{
			myComboBox.SetRedraw(false);
			myComboBox.SetMinVisible(1);
			for (Fraction x : myValues)
				myComboBox.AddString(x.toPrecisionString(4));
			selectOrDefaultAsFraction(myDefault);
			myComboBox.SetMinVisible(static_cast<int>(myValues.size()));
			myComboBox.SetRedraw(true);
		}

	public:
		selection_handler() {}
		selection_handler(CComboBox comboBox, std::vector<Fraction> values, Fraction defaultValue)
			: myComboBox(comboBox), myValues(values), myDefault(defaultValue)
		{
			init();
		}

		// Returns currently selected index if valids, otherwise returns
		// a defaults value.
		Fraction getSelectionOrDefault()
		{
			int selection = myComboBox.GetCurSel();
			return outOfBounds(selection) ? myDefault : myValues[selection];
		}

		Fraction selectOrDefaultAsFraction(Fraction value)
		{
			for (size_t i = 0; i < myValues.size(); ++i)
			{
				if (myValues[i] == value)
				{
					myComboBox.SetCurSel(static_cast<int>(i));
					return value;
				}
			}

			return myDefault;
		}

		Fraction updateSelection()
		{
			return selectOrDefault(myComboBox.GetCurSel());
		}

		// Returns value associated with selection if it is valid, otherwise
		// returns defaults.
		Fraction selectOrDefault(int selection)
		{
			if (outOfBounds(selection))
				return selectOrDefaultAsFraction(myDefault);

			return selectOrDefaultAsFraction(myValues[selection]);
		}

		// Searches for given value in our list of values and 
		// returns it if present, otherwise returns defaults
		Fraction getOrDefault(Fraction value)
		{
			for (Fraction x : myValues)
			{
				if (x == value)
					return value;
			}

			return myDefault;
		}

		std::vector<CString> getAsStrings(size_t precision)
		{
			std::vector<CString> strings(myValues.size());
			for (Fraction x : myValues)
				strings.push_back(x.toCString(precision));

			return strings;
		}
	};

	class slider_limit_handler
	{
	private:
		CTrackBarCtrl mySlider;
		Fraction myMin;
		Fraction myMax;
		Fraction myStep;
		size_t myPos;
		bool myHasInit;

	public:
		slider_limit_handler() : myHasInit(false), myPos(0) {}
		slider_limit_handler(CTrackBarCtrl slider) : myHasInit(false), mySlider(slider), myPos(0) {}

		Fraction numStepsRequired(Fraction min, Fraction max, Fraction step)
		{
			return (max - min) * (1 / step);
		}

		bool init(Fraction min, Fraction max, Fraction step)
		{
			// If we have never loaded, don't compare to garbage values.
			if (myHasInit && min == myMin && max == myMax && step == myStep)
				return false;

			// We have some choices to make... When the step
			// is too large to represent the minimum value, I say we round up.
			//
			if (step > min)
				min = step;

			Fraction stepsRequired = numStepsRequired(min, max, step);
			uint64_t wholeSteps = stepsRequired.wholePart();

			// If the max value is not a multiple of the step, choose the closest 
			// multiple we can.
			//
			max = min + wholeSteps * step;

			myMin = min;
			myMax = max;
			myStep = step;

			mySlider.SetRangeMin(0);
			mySlider.SetRangeMax(static_cast<int>(wholeSteps), true);
			myPos = 0;
			myHasInit = true;

			return true;
		}

		int getPos()
		{
			return mySlider.GetPos();
		}

		Fraction getValue()
		{
			return myMin + myStep * getPos();
		}

		Fraction setPosByValue(double value)
		{
			return setPosByValue(Fraction::fromDouble(value));
		}

		Fraction setPosByValue(Fraction value)
		{
			value = clamp(value);
			Fraction stepsRequired = numStepsRequired(myMin, value, myStep);
			uint64_t pos = stepsRequired.wholePart();
			mySlider.SetPos(static_cast<int>(pos));
			return myMin + myStep * pos;
		}

		Fraction clamp(Fraction value)
		{
			return max(myMin, min(myMax, value));
		}
	};

	class clamped_slider
	{
	public:

		enum Handler {
			MIN,
			PRECISION,
			MAX
		};

		selection_handler myMin;
		CTrackBarCtrl mySlider;
		CEdit mySliderEdit;
		slider_limit_handler myLimitHandler;
		selection_handler myMax;
		selection_handler myPrecision;
		clamped_slider() {}
		clamped_slider(
			selection_handler min,
			CTrackBarCtrl slider,
			CEdit edit,
			selection_handler max,
			selection_handler precision
		) : myMin(min), mySlider(slider), mySliderEdit(edit), myLimitHandler(slider), myMax(max), myPrecision(precision) {}

		void init(Fraction min, double value, Fraction max, Fraction precision)
		{
			Fraction theMin = myMin.selectOrDefaultAsFraction(min);
			Fraction theMax = myMax.selectOrDefaultAsFraction(max);
			Fraction theStep = myPrecision.selectOrDefaultAsFraction(precision);
			myLimitHandler.init(theMin, theMax, theStep);
			myLimitHandler.setPosByValue(value);
			mySliderEdit.SetWindowTextW(myLimitHandler.getValue().toCString(getPrecision()));
		}

		double getValueAsDouble()
		{
			return getValue().toDouble(10);
		}

		Fraction getValue()
		{
			return myLimitHandler.getValue();
		}

		double onEditApply(double value)
		{
			myLimitHandler.setPosByValue(value);
			Fraction clampedValue = Fraction::fromDouble(value);
			clampedValue = clamp(myMin.getSelectionOrDefault(), clampedValue, myMax.getSelectionOrDefault());
			mySliderEdit.SetWindowTextW(clampedValue.toPrecisionString(8));
			return clampedValue.toDouble(8);
		}

		Fraction onScroll()
		{
			Fraction value = myLimitHandler.getValue();
			mySliderEdit.SetWindowTextW(value.toCString(getPrecision()));
			return value;
		}

		Fraction findClosestPositionForValue(double value)
		{
			Fraction asFraction = Fraction::fromDouble(value);
			asFraction = clamp(myMin.getSelectionOrDefault(), asFraction, myMax.getSelectionOrDefault());
			return asFraction;
		}

		Fraction onChanged(selection_handler handler, Handler handlerType)
		{
			Fraction oldValue = myLimitHandler.getValue();
			Fraction newSelection = handler.updateSelection();

			Fraction min = myMin.getSelectionOrDefault();
			Fraction max = myMax.getSelectionOrDefault();
			Fraction step = myPrecision.getSelectionOrDefault();
			switch (handlerType)
			{
			case MIN: myLimitHandler.init(newSelection, max, step); break;
			case MAX: myLimitHandler.init(min, newSelection, step); break;
			case PRECISION: myLimitHandler.init(min, max, newSelection); break;
			}

			myLimitHandler.setPosByValue(oldValue);
			Fraction potentiallyNewRoundedValue = myLimitHandler.getValue();
			mySliderEdit.SetWindowTextW(potentiallyNewRoundedValue.toCString(getPrecision()));

			return newSelection;
		}

		size_t getPrecision()
		{
			uint64_t wholePart = (Fraction(1) / myPrecision.getSelectionOrDefault()).wholePart();
			return (size_t)log10(wholePart);
		}

		Fraction onMinChanged()
		{
			return onChanged(myMin, MIN);
		}

		Fraction onMaxChanged()
		{
			return onChanged(myMax, MAX);
		}

		Fraction onPrecisionChanged()
		{
			return onChanged(myPrecision, PRECISION);
		}

		Fraction clamp(Fraction minValue, Fraction value, Fraction maxValue)
		{
			return max(minValue, min(value, maxValue));
		}
	};
}

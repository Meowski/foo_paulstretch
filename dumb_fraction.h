#pragma once

#include <atlstr.h>
#include <utility>
#include <cmath>
#include <stack>

namespace pauldsp {

	// This is a very, very basic, with no safety guarantees, bad performance, etc..., fraction implementation 
	// to simplify config code. Don't actually use this for any extended calculations.
	//
	class Fraction {
	private:
		int64_t numerator;
		uint64_t denominator;;

	public:

		static Fraction fromDouble(double value, double epsilon = 1e-14, int64_t maxDenominator = 1 << 29)
		{
			int64_t sign = value < 0 ? -1 : 1;
			value = abs(value);

			// continued fractions
			//
			int64_t	h0 = 1;
			int64_t k0 = 0;
			int64_t h1 = 0;
			int64_t k1 = 1;

			Fraction result((int64_t)floor(value));
			double frac = value - (int64_t)floor(value);
			size_t max_iterations = 60;
			while (frac > epsilon && max_iterations-- > 0)
			{
				double recipricol = 1.0 / frac;
				int64_t whole = (int64_t)floor(recipricol);
				frac = recipricol - whole;
				int64_t h2 = whole * h1 + h0;
				int64_t k2 = whole * k1 + k0;

				if (k2 > maxDenominator)
					break;

				h0 = h1;
				k0 = k1;
				h1 = h2;
				k1 = k2;
			}

			return sign * (result + Fraction(h1, k1));
		}

		Fraction() : Fraction(0, 1) {}

		Fraction(int64_t numerator) : Fraction(numerator, 1) {}

		Fraction(int64_t numerator, uint64_t denominator)
		{
			auto x = numerator;
			auto y = denominator;
			auto d = gcd(std::abs(numerator), denominator);
			this->numerator = numerator / d;
			this->denominator = denominator / d;
		}

		int64_t getNumerator()
		{
			return numerator;
		}

		uint64_t getDenominator()
		{
			return denominator;
		}

		int64_t gcd(uint64_t x, uint64_t y) const
		{
			if (y > x)
				std::swap(x, y);
			while (y > 0)
			{
				x = x % y;
				std::swap(x, y);
			}

			return static_cast<int64_t>(x);
		}

		friend bool operator<(const Fraction& lhs, const Fraction& rhs)
		{
			return lhs.numerator * rhs.denominator < lhs.denominator* rhs.numerator;
		}

		friend inline bool operator>(const Fraction& lhs, const Fraction& rhs)
		{
			return rhs < lhs;
		}

		friend inline bool operator<=(const Fraction& lhs, const Fraction& rhs)
		{
			return !(lhs > rhs);
		}

		friend bool operator>=(const Fraction& lhs, const Fraction& rhs)
		{
			return !(lhs < rhs);
		}

		friend bool operator==(const Fraction& lhs, const Fraction& rhs)
		{
			return (lhs.numerator * rhs.denominator == rhs.numerator * lhs.denominator);
		}

		friend bool operator!=(const Fraction& lhs, const Fraction& rhs)
		{
			return !(lhs == rhs);
		}

		Fraction& operator*=(const Fraction& other)
		{
			// knuth
			auto d1 = gcd(std::abs(numerator), other.denominator);
			auto d2 = gcd(std::abs(other.numerator), denominator);
			numerator = (numerator / d1) * (other.numerator / d2);
			denominator = (denominator / d2) * (other.denominator / d1);
			return *this;
		}

		friend Fraction operator*(Fraction lhs, const Fraction& rhs)
		{
			lhs *= rhs;
			return lhs;
		}

		Fraction& operator/=(const Fraction& other)
		{
			if (other.numerator < 0)
				(*this) *= Fraction(-static_cast<int64_t>(other.denominator), static_cast<uint64_t>(std::abs(other.numerator)));
			else
				(*this) *= Fraction(static_cast<int64_t>(other.denominator), static_cast<uint64_t>(other.numerator));
			return *this;
		}

		Fraction friend operator/(Fraction lhs, const Fraction& rhs)
		{
			lhs /= rhs;
			return lhs;
		}

		Fraction& operator+=(const Fraction& other)
		{
			// knuth
			auto u = this->numerator;
			auto uu = this->denominator;
			auto v = other.numerator;
			auto vv = other.denominator;

			auto d1 = gcd(uu, vv);
			if (d1 == 1)
			{
				this->numerator = u * vv + uu * v;
				this->denominator = uu * vv;
			}
			else
			{
				int64_t t = u * (vv / d1) + v * (uu / d1);
				auto d2 = gcd(std::abs(t), d1);
				this->numerator = t / d2;
				this->denominator = (uu / d1) * (vv / d2);
			}

			return *this;
		}

		friend Fraction operator+(Fraction lhs, const Fraction& rhs)
		{
			lhs += rhs;
			return lhs;
		}

		Fraction& operator-=(const Fraction& other)
		{
			return (*this) += Fraction(-other.numerator, other.denominator);
		}

		friend Fraction operator-(Fraction lhs, const Fraction& rhs)
		{
			lhs -= rhs;
			return lhs;
		}

		Fraction& operator++()
		{
			(*this) += Fraction(1, 1);
			return *this;
		}

		Fraction operator++(int)
		{
			Fraction old = *this;
			operator++();
			return old;
		}

		Fraction& operator--()
		{
			(*this) += Fraction(-1, 1);
			return *this;
		}

		Fraction operator--(int)
		{
			Fraction old = *this;
			operator--();
			return old;
		}

		uint64_t wholePart()
		{
			return std::abs(numerator) / denominator;
		}

		Fraction fractionalPart()
		{
			return Fraction(std::abs(numerator) % denominator, denominator);
		}

		uint64_t extractFractionalDigits(size_t numDigits)
		{
			uint64_t fraction_numerator = std::abs(numerator) % denominator;
			uint64_t output = 0;
			while (numDigits-- > 0)
			{
				fraction_numerator *= 10;
				output = output * 10 + (fraction_numerator / denominator);
				fraction_numerator %= denominator;
			}

			return output;
		}

		double toDouble(size_t precision)
		{
			uint64_t whole_part = std::abs(numerator) / denominator;
			double result = static_cast<double>(whole_part);

			if (precision <= 0)
				return result;

			uint64_t frac_digits = extractFractionalDigits(precision + 1);

			bool shouldRoundUp = frac_digits % 10 >= 5;
			frac_digits = frac_digits / 10;
			frac_digits = shouldRoundUp ? frac_digits + 1 : frac_digits;

			return result + static_cast<double>(frac_digits) / pow(10, precision);
		}

		class DigitsInBase {
		public:
			int64_t wholePart;
			std::deque<uint64_t> fractionalDigits;
			DigitsInBase(Fraction f, uint64_t base, size_t amountFractional, bool shouldRoundUp = false, bool truncateZeros = true)
				: wholePart(0), fractionalDigits(0)
			{
				// Handle the whole number only case specially.
				if (amountFractional == 0)
				{
					Fraction half(1, 2);
					wholePart = f.wholePart() + (shouldRoundUp && f.fractionalPart() >= half ? 1 : 0);
					return;
				}

				Fraction fracBase = Fraction(base);
				wholePart = f.wholePart();
				f = f.fractionalPart();
				size_t digitCount = amountFractional + 1;
				while (digitCount-- > 0)
				{
					f *= fracBase;
					fractionalDigits.push_back(f.wholePart());
					f = f.fractionalPart();
				}

				// Now we walk back and account for any rounding.
				uint64_t carry = 0;
				uint64_t lastDigit = fractionalDigits.back();
				if (shouldRoundUp && Fraction(lastDigit, base) >= Fraction(1, 2))
					carry = 1;
				fractionalDigits.pop_back();

				for (size_t i = fractionalDigits.size(); i-- > 0;)
				{
					uint64_t lastDigit = fractionalDigits[i];
					uint64_t digit = lastDigit + carry;
					fractionalDigits[i] = digit % base;
					carry = digit > base ? 1 : 0;
				}

				if (carry > 0)
					++wholePart;

				// Finally, truncate zeros
				if (!truncateZeros)
					return;

				while (!fractionalDigits.empty())
				{
					if (fractionalDigits.back() != 0)
						return;
					fractionalDigits.pop_back();
				}
			}
		};

		CString toCString(size_t precision, bool roundUp = true, bool truncate = false)
		{
			CString str(L"");
			DigitsInBase digits(*this, 10, precision, roundUp, truncate);
			str.AppendFormat(L"%lli", digits.wholePart);

			if (digits.fractionalDigits.empty())
			{
				return str;
			}
			str.AppendChar(L'.');

			for (uint64_t digit : digits.fractionalDigits)
				str.AppendFormat(L"%llu", digit);

			return str;
		}

		// Returns a string with the number of nonzero digits or up to the
		// given precision. Whichever comes first.
		CString toPrecisionString(size_t precision)
		{
			return toCString(precision, true, true);
		}
	};
}
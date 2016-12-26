#pragma once
#include "stdafx.h"
#include <complex>
#include <random>
#include <iostream>
#include <chrono>
#include "kissfft/kiss_fftr.h"
#include <memory>

extern "C"
{
#include "kissfft/kiss_fftr.h"
}

#define PI 3.1415926f

template <typename T>
class sample_vector
{
public:
	sample_vector(const sample_vector& other)
		: mySize(other.mySize)
	{
		if (other.myValues != nullptr)
		{
			myValues = new T[mySize];
			for (size_t i = 0; i < mySize; i++)
				myValues[i] = other.myValues[i];
		}
		else
			myValues = nullptr;
	}

	sample_vector(sample_vector&& other) noexcept
		: myValues(other.myValues),
		  mySize(other.mySize)
	{
		other.myValues = nullptr;
		other.mySize = 0;
	}

	sample_vector& operator=(sample_vector& other)
	{
		if (this == &other)
			return *this;
		mySize = other.mySize;
		delete[] myValues;

		if (other.myValues != nullptr)
		{
			myValues = new T[mySize];
			for (size_t i = 0; i < mySize; i++)
				myValues[i] = other.myValues[i];
		}
		else
			myValues = nullptr;

		return *this;
	}

	sample_vector& operator=(sample_vector&& other) noexcept
	{
		if (this == &other)
			return *this;

		delete[] myValues;

		myValues = other.myValues;
		mySize = other.mySize;

		other.myValues = nullptr;
		other.mySize = 0;
		return *this;
	}

	sample_vector(size_t size, T (*foo_ptr)(T)) : mySize(size)
	{
		if (size > 0)
		{
			myValues = new T[size];
			apply(foo_ptr);
		}
		else
		{
			myValues = nullptr;
			mySize = 0;
		}
	}

	sample_vector(size_t size) : mySize(size)
	{
		if (size > 0)
			myValues = new T[size];
		else
		{
			myValues = nullptr;
			mySize = 0;
		}
	}

	sample_vector() : myValues(nullptr), mySize(0)
	{
	}

	// create evenly spaced points points over interval [start, end]
	//
	void linspace(float start, float end, size_t number_of_points)
	{
		T step = (end - start) / number_of_points;
		myValues[0] = start;

		size_t i = 1;
		// we do this check for underflow issues.
		//
		for (; i < number_of_points - 1 && myValues[i - 1] + step < end; i++)
			myValues[i] = myValues[i - 1] + step;

		for (; i < number_of_points; i++)
			myValues[i] = end;
	}

	// zero out vector
	//
	void zeros()
	{
		for (size_t i = 0; i < mySize; i++)
			myValues[i] = 0;
	}

	// set as integer range [0, stop)
	//
	void arange(size_t stop)
	{
		for (size_t i = 0; i < stop; i++)
			myValues[i] = static_cast<T>(i);
	}

	// returns a hann window of specified size (in samples)
	//
	void hann_window(size_t window_size_in_samples)
	{
		// 0.5 - cos(arange(windowsize,dtype='float')*2.0*pi/(windowsize-1)) * 0.5
		//
		arange(window_size_in_samples);
		multiply(2.0 * PI * (1.0 / (window_size_in_samples - 1)));
		auto func = [](T t) { return static_cast<T>(cos(static_cast<double>(t))); };
		apply(func);
		multiply(-0.5);
		add(0.5);
	}

	void apply(T (*foo_ptr)(T))
	{
		for (size_t i = 0; i < mySize; i++)
		{
			myValues[i] = foo_ptr(myValues[i]);
		}
	}

	// scalar multiplication
	//
	void multiply(T constant)
	{
		for (size_t i = 0; i < mySize; i++)
			myValues[i] *= constant;
	}

	// multiply by other vector, component wise (not dot product)
	//
	void multiply(sample_vector<T> other)
	{
		for (size_t i = 0; i < mySize; i++)
			myValues[i] *= other[i];
	}

	void add(T constant)
	{
		for (size_t i = 0; i < mySize; i++)
			myValues[i] += constant;
	}

	void add(sample_vector<T> other)
	{
		for (size_t i = 0; i < mySize; i++)
			myValues[i] += other[i];
	}

	void set(T value, size_t index)
	{
		this->operator[](index);
	}

	size_t get_size() const
	{
		return mySize;
	}

	T& operator[](size_t index)
	{
		return myValues[index];
	}

	~sample_vector()
	{
		delete[] myValues;
	}

private:
	T* myValues;
	size_t mySize;
};

class paulstretch
{
public:

	static float step_size(size_t window_size_in_samples, float stretch_rate)
	{
		return (window_size_in_samples / 2.0f) / stretch_rate;
	}

	static sample_vector<audio_sample> combine_windows(size_t window_size_samples, sample_vector<audio_sample> previous, sample_vector<audio_sample> current)
	{
		size_t half_window_size = window_size_samples / 2;
		sample_vector<audio_sample> result(half_window_size);
		for (size_t i = 0; i < half_window_size; i++)
		{
			if (previous.get_size() > i + half_window_size && current.get_size() > i)
				result[i] = previous[i + half_window_size] + current[i];
			else if (current.get_size() > i)
				result[i] = current[i];
			else
				result[i] = 0;
		}

		return result;
	}

	static size_t required_sample_size(float window_size_in_seconds, size_t sample_rate)
	{
		size_t size_in_samples = static_cast<size_t>(window_size_in_seconds * sample_rate);
		size_in_samples = max(size_in_samples, 16);
		return optimize_windowsize(size_in_samples);
	}

	// Note: kiss_fftr scales by nfft/2 while kiss_fftri scales by 2
	//
	static sample_vector<audio_sample> stateless_stretch(float stretch_rate, sample_vector<audio_sample> sample)
	{
		size_t windowSize = sample.get_size();

		sample_vector<audio_sample> window = create_window(windowSize);
		sample.multiply(window);

		kiss_fftr_cfg cfg_fft = kiss_fftr_alloc(windowSize, 0, 0, 0);
		kiss_fftr_cfg cfg_ffti = kiss_fftr_alloc(windowSize, 1, 0, 0);

		if (cfg_fft == NULL || cfg_ffti == NULL)
		{
			kiss_fftr_free(cfg_fft);
			kiss_fftr_free(cfg_ffti);
			// could not allocate space - just give up for now!
			//
			return sample;
		}

		size_t numFreq = (windowSize / 2) + 1;

		std::unique_ptr<audio_sample[]> input_time(new audio_sample[windowSize]);
		std::unique_ptr<kiss_fft_cpx[]> output_freq(new kiss_fft_cpx[numFreq]);

		for (size_t i = 0; i < windowSize; i++)
			input_time[i] = sample[i];


		kiss_fftr(cfg_fft, input_time.get(), output_freq.get());
		kiss_fftr_free(cfg_fft);

		sample_vector<std::complex<audio_sample>> complex_result(numFreq);
		for (size_t i = 0; i < numFreq; i++)
		{
			complex_result[i] = std::complex<double>(output_freq[i].r, output_freq[i].i);
			complex_result[i] = abs(complex_result[i]);
		}

		std::uniform_real_distribution<float> rand(0, 2 * PI);
		std::default_random_engine generator(static_cast<size_t>(std::chrono::system_clock::now().time_since_epoch().count()));
		for (size_t i = 0; i < numFreq; i++)
		{
			std::complex<audio_sample> random_complex = std::complex<audio_sample>(rand(generator), 0) * std::complex<audio_sample>(0, 1);
			complex_result[i] *= exp(random_complex);
		}

		// do inverse transform
		//
		for (size_t i = 0; i < numFreq; i++)
		{
			output_freq[i].r = complex_result[i].real();
			output_freq[i].i = complex_result[i].imag();
		}

		kiss_fftri(cfg_ffti, output_freq.get(), input_time.get());
		kiss_fftr_free(cfg_ffti);

		for (size_t i = 0; i < windowSize; i++)
			sample[i] = input_time[i] * window[i] / windowSize;

		return sample;
	}

	static sample_vector<audio_sample> create_window(size_t window_size_in_samples)
	{
		sample_vector<audio_sample> window(window_size_in_samples);
		window.linspace(-1.0, 1.0, window_size_in_samples);
		window.apply([](audio_sample x) { return x * x; });
		window.multiply(-1);
		window.add(1);
		window.apply([](audio_sample x) { return static_cast<audio_sample>(pow(static_cast<float>(x), 1.25)); });
		return window;
	}

private:

	static size_t optimize_windowsize(size_t windowsize_in_samples)
	{
		size_t original_size = windowsize_in_samples;
		while (true)
		{
			windowsize_in_samples = original_size;
			while (windowsize_in_samples % 2 == 0)
				windowsize_in_samples /= 2;
			while (windowsize_in_samples % 3 == 0)
				windowsize_in_samples /= 3;
			while (windowsize_in_samples % 5 == 0)
				windowsize_in_samples /= 5;

			if (windowsize_in_samples < 2)
				break;
			original_size++;
		}
		return (original_size / 2) * 2; // ensure result is even.
	}
};

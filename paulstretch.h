#pragma once
#include "stdafx.h"
#include <complex>
#include <random>
#include <chrono>
#include "kissfft/kiss_fftr.h"
#include <memory>
#include <queue>

extern "C"
{
#include "kissfft/kiss_fftr.h"
}

#define PI 3.1415926f

struct free_wrapper
{
	void operator()(void* x) { kiss_fftr_free(x); }
};

class AudioBuffer
{
private:
	std::unique_ptr<audio_sample[]> myValues;
	size_t mySize;
public:
	AudioBuffer(const AudioBuffer& other)
		: mySize(other.mySize)
	{											
		if (other.myValues != nullptr)
		{
			myValues = std::unique_ptr<audio_sample[]>(new audio_sample[mySize]);
			for (size_t i = 0; i < mySize; i++)
				myValues[i] = other.myValues[i];
		}
		else
			myValues = std::unique_ptr<audio_sample[]>(nullptr);
	}

	AudioBuffer(AudioBuffer&& other) noexcept : myValues(std::move(other.myValues)), mySize(other.mySize)
	{
		other.mySize = 0;
	}

	AudioBuffer& operator=(AudioBuffer& other)
	{
		if (this == &other)
			return *this;
		mySize = other.mySize;

		if (other.myValues != nullptr)
		{
			audio_sample* samples = new audio_sample[mySize];
			for (size_t i = 0; i < mySize; i++)
				samples[i] = other.myValues[i];
			myValues.reset(samples);
		}
		else
			myValues = nullptr;

		return *this;
	}

	AudioBuffer& operator=(AudioBuffer&& other) noexcept
	{
		if (this == &other)
			return *this;

		myValues = std::move(other.myValues);
		mySize = other.mySize;

		other.mySize = 0;
		return *this;
	}

	explicit AudioBuffer(size_t size) : mySize(size)
	{
		if (size > 0)
			myValues = std::unique_ptr<audio_sample[]>(new audio_sample[size]());
		else
		{
			myValues = nullptr;
			mySize = 0;
		}
	}

	AudioBuffer() : myValues(nullptr), mySize(0)
	{
	}

	// create evenly spaced points points over interval [start, end]
	//
	void linspace(float start, float end)
	{
		audio_sample step = (end - start) / mySize;
		myValues[0] = start;

		size_t i = 1;
		// we do this check for underflow issues.
		//
		for (; i < mySize - 1 && myValues[i - 1] + step < end; i++)
			myValues[i] = myValues[i - 1] + step;

		for (; i < mySize; i++)
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
			myValues[i] = static_cast<audio_sample>(i);
	}

	// returns a hann window of specified size (in samples)
	//
	void hann_window(size_t window_size_in_samples)
	{
		// 0.5 - cos(arange(windowsize,dtype='float')*2.0*pi/(windowsize-1)) * 0.5
		//
		arange(window_size_in_samples);
		multiply(2.0f * PI * (1.0f / (window_size_in_samples - 1)));
		auto func = [](audio_sample t) { return static_cast<audio_sample>(cos(static_cast<double>(t))); };
		apply(func);
		multiply(-0.5f);
		add(0.5f);
	}

	void apply(audio_sample(*foo_ptr)(audio_sample)) 
	{
		for (size_t i = 0; i < mySize; i++)
		{
			myValues[i] = foo_ptr(myValues[i]);
		}
	}

	// scalar multiplication
	//
	void multiply(audio_sample constant)
	{
		for (size_t i = 0; i < mySize; i++)
			myValues[i] *= constant;
	}

	// multiply by other vector, component wise (not dot product)
	//
	void multiply(const AudioBuffer& other)
	{
		for (size_t i = 0; i < mySize; i++)
			myValues[i] *= other[i];
	}

	void add(audio_sample constant)
	{
		for (size_t i = 0; i < mySize; i++)
			myValues[i] += constant;
	}

	void add(AudioBuffer other)
	{
		for (size_t i = 0; i < mySize; i++)
			myValues[i] += other[i];
	}

	size_t size() const
	{
		return mySize;
	}

	audio_sample get(size_t index)
	{
		return myValues[index];
	}

	void set(size_t index, audio_sample value)
	{
		myValues[index] = value;
	}

	audio_sample& operator[](size_t index) const
	{
		return myValues[index];
	}

	audio_sample* getArrayPointer() const
	{
		return myValues.get();
	}

	bool empty() const
	{
		return mySize <= 0;
	}
};

class NewPaulstretch
{
private:
	std::deque<audio_sample> myBufferedSamples;
	size_t myWindowSizeInSamples;
	AudioBuffer myBuffers[2];
	AudioBuffer myWindow;
	AudioBuffer myOutput;
	int myCurPointer;
	std::uniform_real_distribution<float> myRand;
	std::default_random_engine myGenerator;
	std::unique_ptr<kiss_fftr_state, free_wrapper> myKissFFTInputBuffer;
	std::unique_ptr<kiss_fftr_state, free_wrapper> myKissFFTInverseBuffer;
	float myAccumulatedSteps;	   
						  
public:
	NewPaulstretch(const NewPaulstretch& other) = delete;
	NewPaulstretch& operator=(const NewPaulstretch& other) = delete;
	NewPaulstretch(NewPaulstretch&&) = default;
	NewPaulstretch& operator=(NewPaulstretch&&) = default;
	explicit NewPaulstretch(const float windowSizeInSeconds, const size_t sampleRate) :
		myBufferedSamples(),
		myRand(0, 2 * PI),
		myGenerator(static_cast<size_t>(std::chrono::system_clock::now().time_since_epoch().count())),
		myKissFFTInputBuffer(nullptr),
		myKissFFTInverseBuffer(nullptr),
		myAccumulatedSteps(0)
	{
		myWindowSizeInSamples = this->requiredSampleSize(windowSizeInSeconds, sampleRate);
		for (int i = 0; i < 2; i++)
			myBuffers[i] = AudioBuffer(myWindowSizeInSamples);
		myCurPointer = 0;
		myWindow = AudioBuffer(myWindowSizeInSamples);
		myOutput = AudioBuffer(myWindowSizeInSamples / 2);
		setupWindow();
	}

	void resize(const float windowSizeInSeconds, const size_t sampleRate)
	{
		myWindowSizeInSamples = this->requiredSampleSize(windowSizeInSeconds, sampleRate);
		AudioBuffer newBuffers[2];
		for (int i = 0; i < 2; i++)
			newBuffers[i] = AudioBuffer(myWindowSizeInSamples);
		for (int i = 0; i < 2; i++)
			for (int j = newBuffers[i].size() - 1, k = myBuffers[i].size() - 1; j >= 0 && k >= 0; --j, --k)
				newBuffers[i][j] = myBuffers[i][k];
		myBuffers[0] = newBuffers[0];
		myBuffers[1] = newBuffers[1];
		myWindow = AudioBuffer(myWindowSizeInSamples);
		myOutput = AudioBuffer(myWindowSizeInSamples / 2);
		myKissFFTInputBuffer = nullptr;
		myKissFFTInverseBuffer = nullptr;
		myAccumulatedSteps = 0;
		setupWindow();
	}

	void feed(const audio_sample sample)
	{
		myBufferedSamples.push_back(sample);
	}

	bool canStep() const
	{
		return myWindowSizeInSamples <= myBufferedSamples.size();
	}

	int numSamplesRequiredForStep() const
	{
		return max(0, (int)myWindowSizeInSamples - myBufferedSamples.size());
	}

	AudioBuffer* step(const float stretch_amount)
	{
		copyQueuedSamplesToCurPointer();
		AudioBuffer* buf = stretch();
		if (buf == nullptr)
			return &myOutput;

		myAccumulatedSteps += stepSize(myWindowSizeInSamples, stretch_amount);
		for (; myAccumulatedSteps >= 1.0f && !myBufferedSamples.empty(); --myAccumulatedSteps)
			myBufferedSamples.pop_front();

		combineWindows();
		myCurPointer = 1 - myCurPointer;
		return &myOutput;
	}

private:

	void copyQueuedSamplesToCurPointer()
	{
		for (size_t i = 0; i < myWindowSizeInSamples && i < myBufferedSamples.size(); i++)
			myBuffers[myCurPointer].set(i, myBufferedSamples[i]);;
	}

	void setupWindow()
	{
		myWindow.linspace(-1.0, 1.0);
		myWindow.apply([](audio_sample x) { return x * x; });
		myWindow.multiply(-1);
		myWindow.add(1);
		myWindow.apply([](audio_sample x) { return static_cast<audio_sample>(pow(static_cast<float>(x), 1.25)); });
	}

	static float stepSize(const size_t windowSizeInSamples, const float stretchAmount)
	{
		return (windowSizeInSamples / 2.0f) / stretchAmount;
	}

	static size_t requiredSampleSize(const float windowSizeInSeconds, const size_t sampleRate)
	{
		size_t size_in_samples = static_cast<size_t>(windowSizeInSeconds * sampleRate);
		size_in_samples = max(size_in_samples, 16);
		return optimizeWindowSize(size_in_samples);
	}

	static size_t optimizeWindowSize(size_t windowSizeInSamples)
	{
		size_t original_size = windowSizeInSamples;
		while (true)
		{
			windowSizeInSamples = original_size;
			while (windowSizeInSamples % 2 == 0)
				windowSizeInSamples /= 2;
			while (windowSizeInSamples % 3 == 0)
				windowSizeInSamples /= 3;
			while (windowSizeInSamples % 5 == 0)
				windowSizeInSamples /= 5;

			if (windowSizeInSamples < 2)
				break;
			original_size++;
		}
		return (original_size / 2) * 2; // ensure result is even.
	}

	AudioBuffer* stretch();
	void combineWindows();
};

class paulstretch
{
public:

	static float step_size(size_t window_size_in_samples, float stretch_rate)
	{
		return (window_size_in_samples / 2.0f) / stretch_rate;
	}

	static size_t required_sample_size(float window_size_in_seconds, size_t sample_rate)
	{
		size_t size_in_samples = static_cast<size_t>(window_size_in_seconds * sample_rate);
		size_in_samples = max(size_in_samples, 16);
		return optimize_windowsize(size_in_samples);
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

void NewPaulstretch::combineWindows()
{
	size_t half_window_size = myOutput.size();
	for (size_t i = 0; i < half_window_size; i++)
		myOutput[i] = myBuffers[1 - myCurPointer].get(i + half_window_size) + myBuffers[myCurPointer].get(i);
}

// Note: kiss_fftr scales by nfft/2 while kiss_fftri scales by 2
//
AudioBuffer* NewPaulstretch::stretch()
{
	
	myBuffers[myCurPointer].multiply(myWindow);;

	if (myKissFFTInputBuffer == nullptr)
	{
		kiss_fftr_cfg cfg_fft = kiss_fftr_alloc(myWindowSizeInSamples, 0, 0, 0);
		if (cfg_fft == NULL)
			return nullptr;
		myKissFFTInputBuffer = std::unique_ptr<kiss_fftr_state, free_wrapper>(cfg_fft);
	}
	if (myKissFFTInverseBuffer == nullptr)
	{
		kiss_fftr_cfg cfg_ffti = kiss_fftr_alloc(myWindowSizeInSamples, 1, 0, 0);
		if (cfg_ffti == NULL)
			return nullptr;
		myKissFFTInverseBuffer = std::unique_ptr<kiss_fftr_state, free_wrapper>(cfg_ffti);
	}

	size_t numFreq = (myWindowSizeInSamples / 2) + 1;
	std::unique_ptr<kiss_fft_cpx[]> output_freq(new kiss_fft_cpx[numFreq]);

	kiss_fftr(myKissFFTInputBuffer.get(), myBuffers[myCurPointer].getArrayPointer(), output_freq.get());

	std::vector<std::complex<audio_sample>> complex_result(numFreq);
	for (size_t i = 0; i < numFreq; i++)
	{
		complex_result[i] = std::complex<double>(output_freq[i].r, output_freq[i].i);
		complex_result[i] = abs(complex_result[i]);
		std::complex<audio_sample> random_complex = std::complex<audio_sample>(myRand(myGenerator), 0) * std::complex<audio_sample>(0, 1);
		complex_result[i] *= exp(random_complex);
	}

	// do inverse transform
	//
	for (size_t i = 0; i < numFreq; i++)
	{
		output_freq[i].r = complex_result[i].real();
		output_freq[i].i = complex_result[i].imag();
	}

	kiss_fftri(myKissFFTInverseBuffer.get(), output_freq.get(), myBuffers[myCurPointer].getArrayPointer());

	for (size_t i = 0; i < myWindowSizeInSamples; i++)
		myBuffers[myCurPointer].set(i, myBuffers[myCurPointer].get(i) * myWindow[i] / myWindowSizeInSamples);

	return &myBuffers[myCurPointer];
}						 
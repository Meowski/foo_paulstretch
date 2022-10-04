#pragma once	

#include <SDK/foobar2000.h>
#include <complex>
#include <random>
#include <chrono>
#include <memory>
#include <queue>

#include <kissfft/kissfft.hh>

constexpr auto PI = 3.14159265358979323846264338327;

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

	AudioBuffer& operator=(const AudioBuffer& other)
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

	/**
	 * \brief sets all bits to zero.
	 */
	void clear()
	{
		multiply(0);
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
	std::uniform_real_distribution<double> myRand;
	std::default_random_engine myGenerator;
	kissfft<audio_sample> myKissFFTR;
	kissfft<audio_sample> myKissFFTRI;
	float myAccumulatedSteps;

public:
	NewPaulstretch(const NewPaulstretch& other) = delete;
	NewPaulstretch& operator=(const NewPaulstretch& other) = delete;
	NewPaulstretch(NewPaulstretch&&) = default;
	NewPaulstretch& operator=(NewPaulstretch&&) = default;
	explicit NewPaulstretch(const float windowSizeInSeconds, const size_t sampleRate) :
		myBufferedSamples(),
		myRand(0, 2 * PI),
		myWindowSizeInSamples(requiredSampleSize(windowSizeInSeconds, sampleRate)),
		myKissFFTR(myWindowSizeInSamples >> 1, false),
		myKissFFTRI(myWindowSizeInSamples >> 1, true),
		myGenerator(static_cast<size_t>(std::chrono::system_clock::now().time_since_epoch().count())),
		myAccumulatedSteps(0)
	{
		for (auto& myBuffer : myBuffers)
			myBuffer = AudioBuffer(myWindowSizeInSamples);
		myCurPointer = 0;
		myWindow = AudioBuffer(myWindowSizeInSamples);
		myOutput = AudioBuffer(myWindowSizeInSamples / 2);
		setupWindow();
	}

	void resize(const float windowSizeInSeconds, const size_t sampleRate)
	{
		myWindowSizeInSamples = this->requiredSampleSize(windowSizeInSeconds, sampleRate);
		myKissFFTR = kissfft<audio_sample>(myWindowSizeInSamples >> 1, false);
		myKissFFTRI = kissfft<audio_sample>(myWindowSizeInSamples >> 1, true);
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
		myAccumulatedSteps = 0;
		myBufferedSamples.clear();
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

	size_t numSamplesRequiredForStep() const
	{
		return max(0, myWindowSizeInSamples - myBufferedSamples.size());
	}

	size_t numBufferedSamples() {
		return myBufferedSamples.size();
	}

	void feedUntilStep(audio_sample sample) {
		size_t req = numSamplesRequiredForStep();
		for (size_t i = 0; i < req; ++i)
			myBufferedSamples.push_back(sample);
	}

	AudioBuffer* step(const float stretch_amount)
	{
		copyQueuedSamplesToCurPointer();
		AudioBuffer* buf = stretch();
		if (buf == nullptr)
			return &myOutput;

		myAccumulatedSteps += stepSize(myWindowSizeInSamples, stretch_amount);
		size_t intSteps = static_cast<size_t>(floor(myAccumulatedSteps));
		size_t elementsToRemove = min(myBufferedSamples.size(), intSteps);
		myBufferedSamples.erase(myBufferedSamples.begin(), myBufferedSamples.begin() + elementsToRemove);
		myAccumulatedSteps -= elementsToRemove;

		combineWindows();
		myCurPointer = 1 - myCurPointer;
		return &myOutput;
	}

	size_t finalStretchesRequired(float stretchAmount)
	{
		if (myBufferedSamples.empty())
			return 0;
		return static_cast<size_t>(ceil(myBufferedSamples.size() / stepSize(myWindowSizeInSamples, stretchAmount)));
	}

	void flush()
	{
		myBuffers[0].clear();
		myBuffers[1].clear();
		myBufferedSamples.clear();
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

inline void NewPaulstretch::combineWindows()
{
	const size_t half_window_size = myOutput.size();
	for (size_t i = 0; i < half_window_size; i++)
		myOutput[i] = myBuffers[1 - myCurPointer].get(i + half_window_size) + myBuffers[myCurPointer].get(i);
}

// Note: kiss_fftr scales by nfft/2 while kiss_fftri scales by 2
//
inline AudioBuffer* NewPaulstretch::stretch()
{

	myBuffers[myCurPointer].multiply(myWindow);;

	size_t numFreq = (myWindowSizeInSamples / 2) + 1;
	std::vector<std::complex<audio_sample>> frequencies(numFreq);
	myKissFFTR.transform_real(myBuffers[myCurPointer].getArrayPointer(), frequencies.data());
	frequencies[numFreq - 1] = std::complex<audio_sample>(frequencies[0].imag(), 0);
	frequencies[0].imag(0);

	for (size_t i = 0; i < numFreq; i++)
	{
		frequencies[i] = abs(frequencies[i]);
		std::complex<audio_sample> random_complex = std::complex<audio_sample>(myRand(myGenerator), 0) * std::complex<audio_sample>(0, 1);
		frequencies[i] *= exp(random_complex);
	}

	myKissFFTRI.transform_real_inverse(frequencies.data(), myBuffers[myCurPointer].getArrayPointer());

	for (size_t i = 0; i < myWindowSizeInSamples; i++)
		myBuffers[myCurPointer].set(i, myBuffers[myCurPointer].get(i) * myWindow[i] / myWindowSizeInSamples);

	return &myBuffers[myCurPointer];
}
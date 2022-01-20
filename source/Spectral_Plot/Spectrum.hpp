#ifndef __HISSTOOLS_SPECTRUM__
#define __HISSTOOLS_SPECTRUM__

#include "../HISSTools_Library/HISSTools_FFT/HISSTools_FFT.h"

class Spectrum
{

public:
    
	Spectrum(unsigned long maxFFTSize)
	{
		// Check FFT Size is power of two for allocation
		
		maxFFTSize = 1 << log2(maxFFTSize);
		
		unsigned long requiredPointerSize = calcMaxBin(maxFFTSize);
		
		// Allocate memory assuming failure
		// This might need to be altered later to ensure 16 byte alignment!!
		
		mPointerSize = 0;
		mFFTSize = 0;
		mSamplingRate = 0;
		
		mSpectrum.realp = new double[requiredPointerSize];
		mSpectrum.imagp = new double[requiredPointerSize];
		
		if (mSpectrum.realp && mSpectrum.imagp)
		{
			mPointerSize = requiredPointerSize;
			mFFTSize = maxFFTSize;
		}
	}
	
	~Spectrum()
	{
		delete[] mSpectrum.realp;
		delete[] mSpectrum.imagp;
	}
	
	bool copy(Spectrum *inSpectrum)
	{
		// Attempt to set size
		
		if (setParams(inSpectrum->getFFTSize(), inSpectrum->getSamplingRate()) == false)
			return false;
		
		// Copy spectrum
		
        unsigned long copySize = (inSpectrum->getFFTSize() >> 1);
        
        memcpy(getSpectrum()->realp, inSpectrum->getSpectrum()->realp, copySize * sizeof(double));
        memcpy(getSpectrum()->imagp, inSpectrum->getSpectrum()->imagp, copySize * sizeof(double));
        
		return true;
	}
	
	bool setParams(unsigned long FFTSize, double samplingRate)
	{
		// Check pointer size
		
		if (calcMaxBin(FFTSize) > mPointerSize)
			return false;
		
		// Set parameters
		
		mFFTSize = FFTSize;
		
		setSamplingRate(samplingRate);
		
		return true;
	}
	
	bool setFFTSize(unsigned long FFTSize)      { return setParams(FFTSize, mSamplingRate); }
	void setSamplingRate(double samplingRate)   { mSamplingRate = fabs(samplingRate); }
	
    FFT_SPLIT_COMPLEX_D *getSpectrum()          { return &mSpectrum; }
	unsigned long getFFTSize() const            { return mFFTSize; }
	unsigned long getMaxBin() const             { return calcMaxBin(mFFTSize); }
	double getSamplingRate() const              { return mSamplingRate; }
		
private:
	
    static unsigned long calcMaxBin(unsigned long FFTSize) { return (FFTSize + 1) >> 1; }
    
    static unsigned long log2 (unsigned long in)
    {
        unsigned long temp = in;
        unsigned long out = 0;
        
        while (temp)
        {
            temp >>= 1;
            out++;
        }
        
        if (in == 1 << (out - 1))
            return out - 1;
        else 
            return out;
    }

	// The Spectrum
	
	FFT_SPLIT_COMPLEX_D mSpectrum;
	
	// Parameters
		
	unsigned long mFFTSize;
	unsigned long mPointerSize;
	
	// Sampling Rate (drawing/analysis etc.)
	
	double mSamplingRate;
};

#endif

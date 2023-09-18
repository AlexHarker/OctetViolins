

#ifndef __HISSTOOLS_POWERSPECTRUM__
#define __HISSTOOLS_POWERSPECTRUM__


#include <fft/fft.hpp>
#include "Spectrum.hpp"
#include <vector>

class PowerSpectrum
{
	
public:

	PowerSpectrum(unsigned long maxFFTSize)
	{
		// Allocate memory
				
		mSpectrum.resize(calcMaxBin(maxFFTSize));
		mFFTSize = 0;
		mSamplingRate = 0.;
	}
	
	bool copy(PowerSpectrum *inSpectrum)
	{
		unsigned long FFTSize = inSpectrum->getFFTSize();
		
		// Attempt to set size
		
		if (setParams(FFTSize, inSpectrum->getSamplingRate()) == false)
			return false;
		
		// Copy spectrum
		
        memcpy(getSpectrum(), inSpectrum->getSpectrum(), inSpectrum->getMaxBin() * sizeof(double));
						
		return true;
	}
	
	bool calcPowerSpectrum(Spectrum *inSpectrum, double scale = 0.)
	{
        htl::split_type<double> in = *inSpectrum->getSpectrum();
		
		double *spectrum = getSpectrum();
		unsigned long FFTSize = inSpectrum->getFFTSize();
		
		
		scale = scale == 0 ? 1 : scale;

		// Attempt to set size
		
		if (setFFTSize(FFTSize) == false)
			return false;
		
        unsigned long endBin = std::min(getMaxBin(), inSpectrum->getMaxBin());
        
		// Do DC
        
        spectrum[0] = in.realp[0] * in.realp[0] * scale;
        
		// Loop over spectrum
		
		for (long i = 1; i < endBin; i++)
			spectrum[i] = (in.realp[i] * in.realp[i] + in.imagp[i] * in.imagp[i]) * scale;
				
        // Do Nyquist
		
		spectrum[FFTSize >> 1] = (in.imagp[0] * in.imagp[0]) * scale;
		
		setSamplingRate(inSpectrum->getSamplingRate());
		
		return true;
	}
	
	bool setParams(unsigned long FFTSize, double samplingRate)
	{
		// Check size

		if (calcMaxBin(FFTSize) > mSpectrum.size())
			return false;
		
		// Set parameters
		
		mFFTSize = FFTSize;
		setSamplingRate(samplingRate);
		
		return true;
	}

    bool setFFTSize(unsigned long FFTSize)      { return setParams(FFTSize, mSamplingRate); }
	void setSamplingRate(double samplingRate)   { mSamplingRate = fabs(samplingRate); }
	
	double *getSpectrum()                   { return &mSpectrum[0]; }
    unsigned long getFFTSize() const        { return mFFTSize; }
    unsigned long getMaxBin() const         { return calcMaxBin(mFFTSize); }
	double getSamplingRate() const          { return mSamplingRate; }
	
	
private:
    
    static unsigned long calcMaxBin(unsigned long FFTSize)  { return ((FFTSize + 1) >> 1) + 1; }
	
	// The Spectrum 
	
    std::vector<double> mSpectrum;
	
	unsigned long mFFTSize;
	
	// Sampling Rate (drawing/analysis etc.)
	
	double mSamplingRate;
};

#endif

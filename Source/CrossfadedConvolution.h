

#ifndef __CROSSFADEDCONVOLUTION_
#define __CROSSFADEDCONVOLUTION_

#include "Convolver.h"


class CrossfadedConvolution
{
    class FadedConvolution   
    {
    
    public:
        
        FadedConvolution(): mConvolver(1, 2, HISSTools::Convolver::kLatencyZero), mOn (false), mFade(0) {}

        void set(const double *IRL, const double *IRR, long length);
        void process(const double **inputs, double **outputs, int nFrames);
        void reset()    { mConvolver.reset(); }
        
        void setSamplingRate(double samplingRate) { mSamplingRate = samplingRate; }
        
        void fade()                 { mOn = false; }
        bool busy()                 { return mFade != 0 || mOn; }
        
    private:
        
        HISSTools::Convolver mConvolver;
        double mSamplingRate;
        bool mOn;
        int mFade;
    };
    
public:
    
    CrossfadedConvolution() : mFree(0) {}
    
    bool set(const double *IRL, const double *IRR, long length);
    void process(const double **inputs, double **outputs, int nFrames);
    void reset(double samplingRate);
    
private:

    FadedConvolution mConvolvers[2];
    int mFree;
    
};

#endif
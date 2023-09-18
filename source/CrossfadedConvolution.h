

#ifndef __CROSSFADEDCONVOLUTION_
#define __CROSSFADEDCONVOLUTION_

#include <convolution/multichannel.hpp>


class CrossfadedConvolution
{
    class FadedConvolution   
    {
    public:
        
        FadedConvolution(): mConvolver(1, 2, htl::latency_mode::zero), mOn (false), mFade(0) {}

        void set(const double *IRL, const double *IRR, long length);
        void process(const double **inputs, double **outputs, int nFrames);
        void reset()    { mConvolver.reset(); }
        
        void setSamplingRate(double samplingRate) { mSamplingRate = samplingRate; }
        
        void fade()                 { mOn = false; }
        bool busy()                 { return mFade != 0 || mOn; }
        
    private:
        
        htl::convolve_multichannel<float, double> mConvolver;
        double mSamplingRate;
        bool mOn;
        int mFade;
    };
    
public:
    
    CrossfadedConvolution() : mTemporary(1024), mFree(0) {}
    
    bool set(const double *IRL, const double *IRR, long length);
    void process(const double **inputs, double **outputs, int nFrames);
    void reset(double samplingRate);
    
private:

    std::vector<double> mTemporary;
    FadedConvolution mConvolvers[2];
    int mFree;
};

#endif

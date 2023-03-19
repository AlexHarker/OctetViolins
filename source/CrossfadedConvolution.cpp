

#include "CrossfadedConvolution.h"

#include <algorithm>
#include <cmath>

void CrossfadedConvolution::FadedConvolution::set(const double *IRL, const double *IRR, long length)
{
    mConvolver.set(0, 0, IRL, length, true);
    mConvolver.set(0, 1, IRR, length, true);
    
    mOn = true;
}

void CrossfadedConvolution::FadedConvolution::process(const double **inputs, double **outputs, int nFrames)
{
    int fadeSamps = std::round(0.1 * mSamplingRate);

    mConvolver.process(inputs, outputs, 1, 2, nFrames);

    for (int i = 0; i < nFrames; i++)
    {
        mFade = mOn && mFade < fadeSamps ? mFade + 1 : mFade;
        mFade = !mOn && mFade > 0 ? mFade - 1 : mFade;
        
        double fade = mFade / (double) fadeSamps;
        
        outputs[0][i] *= fade;
        outputs[1][i] *= fade;
    }
}

bool CrossfadedConvolution::set(const double *IRL, const double *IRR, long length)
{
    if (mConvolvers[mFree].busy())
        return false;
    
    mConvolvers[mFree].set(IRL, IRR, length);
    mFree = 1 - mFree;
    mConvolvers[mFree].fade();
    
    return true;
}

void CrossfadedConvolution::process(const double **inputs, double **outputs, int nFrames)
{
    if ((nFrames * 2) > mTemporary.size())
      mTemporary.resize(nFrames * 2);
  
    double *temps[2];
    
    temps[0] = mTemporary.data();
    temps[1] = temps[0] + nFrames;
    
    std::fill_n(outputs[0], nFrames, 0.0);
    std::fill_n(outputs[1], nFrames, 0.0);
    
    for (int i = 0; i < 2; i++)
    {
        if (mConvolvers[i].busy())
        {
            mConvolvers[i].process(inputs, temps, nFrames);
            
            for (int j = 0; j < nFrames; j++)
                outputs[0][j] += temps[0][j];
            for (int j = 0; j < nFrames; j++)
                outputs[1][j] += temps[1][j];
        }
    }
}

void CrossfadedConvolution::reset(double samplingRate)
{
    for (int i = 0; i < 2; i++)
    {
        mConvolvers[i].reset();
        mConvolvers[i].setSamplingRate(samplingRate);
    }
}

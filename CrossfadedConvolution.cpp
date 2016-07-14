

#include "CrossfadedConvolution.h"


void CrossfadedConvolution::FadedConvolution::set(const double *IRL, const double *IRR, long length)
{
    mConvolver.set(0, 0, IRL, length, true);
    mConvolver.set(0, 1, IRR, length, true);
    
    mOn = true;
}

void CrossfadedConvolution::FadedConvolution::process(const double **inputs, double **outputs, int nFrames)
{
    int fadeSamps = 8000;

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
    // FIX - later
    
    double tempL[nFrames];
    double tempR[nFrames];
    double *temps[2];
    
    temps[0] = tempL;
    temps[1] = tempR;
    
    memset(outputs[0], 0, sizeof(double) * nFrames);
    memset(outputs[1], 0, sizeof(double) * nFrames);
    
    for (int i = 0; i < 2; i++)
    {
        if (mConvolvers[i].busy())
        {
            mConvolvers[i].process(inputs, temps, nFrames);
            
            for (int j = 0; j < nFrames; j++)
                outputs[0][j] += tempL[j];
            for (int j = 0; j < nFrames; j++)
                outputs[1][j] += tempR[j];
        }
    }
}

void CrossfadedConvolution::reset()
{
    for (int i = 0; i < 2; i++)
        mConvolvers[i].reset();
}
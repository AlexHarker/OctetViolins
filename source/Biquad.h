
#ifndef __BIQUAD_H__
#define __BIQUAD_H__

#include <vector>
#include <math.h>

// Biquad Filter

template <typename T> struct Biquad
{
    struct Coefficients
    {
        Coefficients(T A1 = 0.0, T A2 = 0.0, T B0 = 0.0, T B1 = 0.0, T B2 = 0.0)
        : a1(A1), a2(A2), b0(B0), b1(B1), b2(B2){}
        
        T a1, a2, b0, b1, b2;
    };
    
    Biquad()
    {
        Coefficients coeff;
        
        reset();
        set(coeff);
    }

    inline T operator()(T x)
    {
        T y = (x * mCoeff.b0) + (x1 * mCoeff.b1) + (x2 * mCoeff.b2) - (y1 * mCoeff.a1) - (y2 * mCoeff.a2);
        
        x2 = x1;
        x1 = x;
        y2 = y1;
        y1 = y;
        
        return y;
    }
    
    inline T operator()(T x, const Coefficients& coeff)
    {
        set(coeff);
        return operator()(x);
    }

    void set(const Coefficients& coeff)
    {
        mCoeff = coeff;
    }
    
    void reset()
    {
        x1 = x2 = y1 = y2 = 0.0;
    }
    
private:
    
    Coefficients mCoeff;
    T x1, x2, y1, y2;
};

typedef Biquad<double> MonoBiquad;

// Coefficient Helpers

static inline MonoBiquad::Coefficients convertCoefficients (double a0, double a1, double a2, double b0, double b1, double b2)
{
    MonoBiquad::Coefficients coeff;
    
    double g = 1.0 / a0;
    
    coeff.a1 = a1 * g;
    coeff.a2 = a2 * g;
    coeff.b0 = b0 * g;
    coeff.b1 = b1 * g;
    coeff.b2 = b2 * g;
    
    return coeff;
}

// Damp Shelving Coefficients

static inline Biquad<double>::Coefficients dampLSF(double f0, double dBGain, double Q, double samplingRate)
{
    double a0, a1, a2, b0, b1, b2;

    double A        = pow(10.0, (dBGain / 40.0));
    double w0       = 2.0 * M_PI * f0 / samplingRate;
    double alpha 	= sin(w0) / (2.0 * Q);

    double cosW0    = cos(w0);
    double sqrtA    = sqrt(A);
    
    b0 =	A * ((A + 1.0) - (A - 1.0) * cosW0 + 2.0 * sqrtA * alpha);
    b1 =	2.0 * A * ((A - 1.0) - (A + 1.0) * cosW0);
    b2 =	A * ((A + 1.0) - (A - 1.0) * cosW0 - 2.0 * sqrtA * alpha);
    a0 =	(A + 1.0) + (A - 1.0) * cosW0 + 2.0 * sqrtA * alpha;
    a1 =	-2.0 * ((A - 1.0) + (A + 1.0) * cosW0);
    a2 =	(A + 1.0) + (A - 1.0) * cosW0 - 2.0 * sqrtA * alpha;
        
    return convertCoefficients(a0, a1, a2, b0, b1, b2);
}


static inline Biquad<double>::Coefficients dampHSF(double f0, double dBGain, double Q, double samplingRate)
{
    double a0, a1, a2, b0, b1, b2;
    
    double A        = pow(10.0, (dBGain / 40.0));
    double w0       = 2.0 * M_PI * f0 / samplingRate;
    double alpha 	= sin(w0) / (2.0 * Q);
    
    double cosW0    = cos(w0);
    double sqrtA    = sqrt(A);
    
    b0 =	A * ((A + 1.0) + (A - 1.0) * cosW0 + 2.0 * sqrtA * alpha);
    b1 =	-2.0 * A * ((A - 1.0) + (A + 1.0) * cosW0);
    b2 =	A * ((A + 1.0) + (A - 1.0) * cosW0 - 2.0 * sqrtA * alpha);
    a0 =	(A + 1.0) - (A - 1.0) * cosW0 + 2.0 * sqrtA * alpha;
    a1 =	2.0 * ((A - 1.0) - (A + 1.0) * cosW0);
    a2 =	(A + 1.0) - (A - 1.0) * cosW0 - 2.0 * sqrtA * alpha;
        
    return convertCoefficients(a0, a1, a2, b0, b1, b2);
}

// Damp Shelving Coefficients Helper

struct shelfCoefficients
{
    MonoBiquad::Coefficients calcLSF(double freq, double db, double s, double samplingRate)
    {
        if (freq != mFreq || db != mDB || s != mS || samplingRate != mSamplingRate)
        {
            mCoefficients = dampLSF(freq, db, s, samplingRate);
            mFreq = freq;;
            mDB = db;
            mS = s;
            mSamplingRate = samplingRate;
        }
        
        return mCoefficients;
    }
    
    MonoBiquad::Coefficients calcHSF(double freq, double db, double s, double samplingRate)
    {
        if (freq != mFreq || db != mDB || s != mS || samplingRate!= mSamplingRate)
        {
            mCoefficients = dampHSF(freq, db, s, samplingRate);
            mFreq = freq;;
            mDB = db;
            mS = s;
            mSamplingRate = samplingRate;
        }
        
        return mCoefficients;
    }
    
private:
    
    double mFreq;
    double mDB;
    double mS;
    double mSamplingRate;
    MonoBiquad::Coefficients mCoefficients;
};


// Butterworth Filters

template <typename T> struct ButterworthLPF
{    
    ButterworthLPF(unsigned long order = 8) : mOrder(0), mCutoff(0.0), mSamplingRate(0.0)
    {
        mBiquads.resize((order + 1) / 2);
    }
    
    inline T operator()(T x)
    {
        for (unsigned long i = 0; i < mN; i++)
            x = mBiquads[i](x);
        
        return x;
    }
    
    inline T operator()(T x, const double cutoff, unsigned long order, double samplingRate)
    {
        set(cutoff, order, samplingRate);
        return operator()(x);
    }
    
    void set(const double cutoff, unsigned long order, double samplingRate)
    {
        order = order > (mBiquads.size() << 1) ? (mBiquads.size() << 1) : order;
        
        if (order == mOrder && cutoff == mCutoff && samplingRate == mSamplingRate)
            return;
        
        mOrder = order;
        mN = (order + 1) / 2;
        mCutoff = cutoff;
        mSamplingRate = samplingRate;
        
        double cf = tan(M_PI * cutoff / samplingRate);
        double cf2 = cf * cf;
        
        for (unsigned int i = 0; i < (order / 2); i++)
        {
            double p = 2.0 * cf * cos(M_PI * (order + 2 * (i + 1) - 1) / (2 * order));
            mBiquads[i].set(convertCoefficients(T(1.0 - p + cf2), T(2.0 * (cf2 - 1.0)), T(cf2 + p + 1.0), T(cf2), T(2.0 * cf2), T(cf2)));
        }
        
        if (order % 2)
            mBiquads[mN - 1].set(convertCoefficients(T(1.0 + cf), T(cf - 1.0), T(0.0), T(cf), T(cf), T(0.0)));
    }

    
    void reset()
    {
        for (typename std::vector<Biquad <T> >::iterator it = mBiquads.begin(); it != mBiquads.end(); it++)
            it->reset();
    }
    
private:
    
    std::vector<Biquad <T> > mBiquads;

    unsigned long mOrder;
    unsigned long mN;
    
    double mSamplingRate;
    double mCutoff;
};

template <typename T> struct ButterworthHPF
{
    ButterworthHPF(unsigned long order = 8) : mOrder(0), mCutoff(0.0), mSamplingRate(0.0)
    {
        mBiquads.resize((order + 1) / 2);
    }
    
    inline T operator()(T x)
    {
        for (unsigned long i = 0; i < mN; i++)
            x = mBiquads[i](x);
        
        return x;
    }
    
    inline T operator()(T x, const double cutoff, unsigned long order, double samplingRate)
    {
        set(cutoff, order, samplingRate);
        return operator()(x);
    }
    
    void set(const double cutoff, unsigned long order, double samplingRate)
    {
        order = order > (mBiquads.size() << 1) ? (mBiquads.size() << 1) : order;
        
        if (order == mOrder && cutoff == mCutoff && samplingRate == mSamplingRate)
            return;
        
        mOrder = order;
        mN = (order + 1) / 2;
        mCutoff = cutoff;
        mSamplingRate = samplingRate;
        
        double cf = tan(M_PI * cutoff / samplingRate);
        double cf2 = cf * cf;
        double cf3 = cf2 * cf;
        
        for (unsigned int i = 0; i < (order / 2); i++)
        {
            double p = 2.0 * cf2 * cos(M_PI * (order + 2 * (i + 1) - 1) / (2 * order));
            mBiquads[i].set(convertCoefficients(T(cf - p + cf3), T(2.0 * (cf3 - cf)), T(cf3 + p + cf), T(cf), T(-2.0 * cf), T(cf)));
        }
        
        if (order % 2)
            mBiquads[mN - 1].set(convertCoefficients(T(1.0 + cf), T((cf - 1.0)), T(0.0), T(1.0), T(-1.0), T(0.0)));
    }
    
    
    void reset()
    {
        for (typename std::vector<Biquad <T> >::iterator it = mBiquads.begin(); it != mBiquads.end(); it++)
            it->reset();
    }
    
private:
    
    std::vector<Biquad <T>> mBiquads;
    
    unsigned long mOrder;
    unsigned long mN;
    
    double mSamplingRate;
    double mCutoff;
};


typedef ButterworthLPF<double> MonoButterworthLPF;

#endif

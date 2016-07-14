
#include <assert.h>

#include "AH_VectorOps.h"

// FIX - add memory allocation issue handling and replace calls to malloc if possible
// N.B. - clean up for other usage (template / different phases / consider other windows etc.)

class Resampler
{
    
public:
    
    Resampler() : mFilter(NULL)
    {
        setFilter(10, 16384, 0.455, 11.0);
    }

    ~Resampler()
    {
        delete[] mFilter;
    }

private:

    // Make filter
    
    double IZero(double x2)
    {
        double term = 1.0;
        double bessel = 1.0;
        
        // N.B. - loop until term is zero for maximum accuracy
        
        for (unsigned long i = 1; term; i++)
        {
            term = term * x2 * (1.0 / (4.0 * (double) i * (double) i));
            bessel += term;
        }
        
        return bessel;
    }

    void setFilter(unsigned long numZeros, unsigned long numPoints, double cf, double alpha)
    {
        assert(numZeros != 0 && "resampler: number of zeros cannot be zero");
        assert(numPoints != 0 && "resampler: number of points per zero cannot be zero");
        
        if (alpha <= 0.0)
            alpha = 1.0;
        
        unsigned long halfFilterLength = numZeros * numPoints;
        double oneOverBesselOfAlpha, val, sincArgument;
        double *filter = new double[halfFilterLength + 2];
        
        // First find bessel function of alpha
        
        oneOverBesselOfAlpha = 1.0 / IZero(alpha * alpha);
        
        // Calculate second half of filter only
        
        // Limit Value
        
        filter[0] = 2.0 * cf;
        
        for (unsigned long i = 1; i < halfFilterLength + 1; i++)
        {
            // Kaiser Window
            
            val = ((double) i) / halfFilterLength;
            val = IZero((1.0 - val * val) * alpha * alpha) * oneOverBesselOfAlpha;
            
            // Multiply with Sinc Function
            
            sincArgument = M_PI * (double) i / numPoints;
            filter[i] = (sin (2 * cf * sincArgument) / sincArgument) * val;
        }
        
        // Guard sample for linear interpolation (N.B. - the max read index should be halfFilterLength)
        
        filter[halfFilterLength + 1] = 0.0;
        
        delete[] mFilter;
        
        mFilter = filter;
        mNumZeros = numZeros;
        mNumPoints = numPoints;
    }

    // Get a filter value from a position 0-nzero on the RHS wing (translate for LHS) - filterPosition **MUST** be in range 0 to nzero inclusive
    
    double getFilterValue(double filterPosition)
    {
        double index, fract, lo, hi;
        long idx;
        
        index = mNumPoints * filterPosition;
        idx = (long) index;
        fract = index - idx;
        
        lo = mFilter[idx];
        hi = mFilter[idx + 1];
        
        return lo + fract * (hi - lo);
    }
    
    double *createTempFilters(long num, long denom, long& filterLength, long& filterOffset)
    {
        double perSample = num > denom ? (double) denom / (double) num : (double) 1;
        double oneOverPerSample = num > denom ? mNumZeros * (double) num / (double) denom : mNumZeros;
        
        // FIX - what is the correct value here!
        
        double ratio = (double) num / (double) denom;
        double mul = ratio > 1.0 ? ratio : 1.0;
        
        filterLength = oneOverPerSample + oneOverPerSample + 1;
        filterOffset = filterLength >> 1;
        
        double *tempFilters;
        double *currentFilter;
        
        filterLength += (4 - (filterLength % 4));
        
        tempFilters = (double *) ALIGNED_MALLOC(denom * filterLength * sizeof(double));
        currentFilter = tempFilters;
        
        for (unsigned long i = 0, currentNum = 0; i < denom; i++, currentFilter += filterLength, currentNum += num)
        {
            while (currentNum >= denom)
                currentNum -= denom;
            
            for (unsigned long j = 0; j < filterLength; j++)
            {
                double filterPosition = fabs(perSample * (j - (double) currentNum / (double) denom - filterOffset));
                currentFilter[j] = filterPosition <= mNumZeros ? mul * getFilterValue(filterPosition) : 0;
            }
        }
        
        return tempFilters;
    }
    
    // This will be redundant if we use padding in the temp....
    
    void safeSamples(float *outBuffer, float *inBuffer, unsigned long inLength, long offset, long nSamps)
    {
        long tempOffset = 0;
        long tempNSamps = nSamps;
        long i;
        
        // Do not read before the buffer
        
        if (offset < 0)
        {
            tempOffset = -offset;
            tempNSamps -= tempOffset;
            offset = 0;
        }
        
        if (tempNSamps < 0)
        {
            tempNSamps = 0;
            tempOffset = nSamps;
        }
        
        // Do not read beyond the buffer
        
        if (offset + tempNSamps > inLength)
            tempNSamps = inLength - offset;
        
        if (tempNSamps < 0)
            tempNSamps = 0;
        
        for (i = 0; i < tempOffset; i++)
            outBuffer[i] = 0;
        
        for (i = 0; i < tempNSamps; i++)
            outBuffer[i + tempOffset] = inBuffer[i + offset];
        
        for (i = tempOffset + tempNSamps; i < nSamps; i++)
            outBuffer[i] = 0;
    }
    
    // Calculate one sample 

    double calcSample(float *input, unsigned long inLength, float *samples, double position, double rate)
    {
        double filterPosition, sum, fract;
        double perSample = rate > 1.0 ? 1.0 / (rate * mNumZeros) : 1.0 / mNumZeros;
        double oneOverPerSample = rate > 1.0 ? mNumZeros * rate : mNumZeros;
        long offset, nsamps, idx;
        
        idx = (long) position;
        fract = position - idx;
        sum = 0.0;
        
        // Get samples
        
        offset = position - oneOverPerSample;
        nsamps = oneOverPerSample + oneOverPerSample + 2;
        safeSamples(samples, input, inLength, offset, nsamps);
        
        // Get to first relevant sample
        
        for (filterPosition = (idx - offset + fract) * perSample; filterPosition > 1.0; filterPosition -= perSample)
            samples++;
        
        // Do left wing of the filter
        
        // N.B. - refactor this later for speed and/or different phases...
        
        for (; filterPosition >= 0.0; filterPosition -= perSample)
            sum += *samples++ * getFilterValue(filterPosition * mNumZeros);
        
        // Do right wing of the filter
        
        for (filterPosition = -filterPosition; filterPosition <= 1.0; filterPosition += perSample)
            sum += *samples++ * getFilterValue(filterPosition * mNumZeros);
        
        return sum;
    }

    float *resampleRate(float *input, unsigned long inLength, double offset, unsigned long nSamps, double rate)
    {
        double oneOverPerSample = rate > 1.0 ? mNumZeros * rate : mNumZeros;
        double mul = rate > 1.0 ? 1.0 / rate : 1.0;
        long tempLength = oneOverPerSample + oneOverPerSample + 2.0;
        
        // Allocate memory
        
        float *temp = new float[tempLength];
        float *output = new float[nSamps];
        
        // Resample
            
        for (unsigned long i = 0; i < nSamps; i++)
            output[i] = mul * calcSample(input, inLength, temp, offset + (i * rate), rate);

        // Free temp memory and return
        
        delete[] temp;
            
        return output;
    }
    
    double applyFilterScalar(double *filter, float *input, unsigned long nSamps)
    {
        double filterSum = 0.0;
        unsigned long i;
            
        for (i = 0; i + 3 < nSamps; i += 4)
        {
            filterSum += filter[i+0] * input[i+0];
            filterSum += filter[i+1] * input[i+1];
            filterSum += filter[i+2] * input[i+2];
            filterSum += filter[i+3] * input[i+3];
        }
        for (; i < nSamps; i++)
            filterSum += filter[i] * input[i];
            
        return filterSum;
    }

    #ifdef TARGET_INTEL
    inline double applyFilterVector(vDouble *filter, float *input, unsigned long nSamps)
    {
        vDouble filterSum = {0.0, 0.0};
        vFloat inputVec;
        double results[2];
        unsigned long i;
            
        for (i = 0; i + 3 < (nSamps >> 1); i += 4)
        {
            inputVec = F32_VEC_ULOAD(input + 2 * i);
            filterSum = F64_VEC_ADD_OP(filterSum, F64_VEC_MUL_OP(filter[i+0], F64_VEC_FROM_F32(inputVec)));
            filterSum = F64_VEC_ADD_OP(filterSum, F64_VEC_MUL_OP(filter[i+1], F64_VEC_FROM_F32(F32_VEC_SHUFFLE(inputVec, inputVec, 0x4E))));
            
            inputVec = F32_VEC_ULOAD(input + 2 * i + 4);
            filterSum = F64_VEC_ADD_OP(filterSum, F64_VEC_MUL_OP(filter[i+2], F64_VEC_FROM_F32(inputVec)));
            filterSum = F64_VEC_ADD_OP(filterSum, F64_VEC_MUL_OP(filter[i+3], F64_VEC_FROM_F32(F32_VEC_SHUFFLE(inputVec, inputVec, 0x4E))));
        }
        for (; i + 1 < nSamps >> 1; i += 2)
        {
            inputVec = F32_VEC_ULOAD(input + 2 * i);
            filterSum = F64_VEC_ADD_OP(filterSum, F64_VEC_MUL_OP(filter[i+0], F64_VEC_FROM_F32(inputVec)));
            filterSum = F64_VEC_ADD_OP(filterSum, F64_VEC_MUL_OP(filter[i+1], F64_VEC_FROM_F32(F32_VEC_SHUFFLE(inputVec, inputVec, 0x4E))));
        }
        
        F64_VEC_USTORE(results, filterSum);
        
        return results[0] + results[1];
    }
    #endif
    
    float *resampleRatio(float *input, unsigned long inLength, long nsamps, long num, long denom)
    {
        long filterOffset;
        long filterLength;
        
        // Create the relevant filters
        
        double *tempFilters = createTempFilters(num, denom, filterLength, filterOffset);
        double *currentFilter;
        
        // Allocate memory
        
        float *output = new float[nsamps];
        float *inputTemp = (float *) ALIGNED_MALLOC((inLength + filterLength) * sizeof(float));
        
        // Copy a padded version of the buffer....
        
        memset(inputTemp, 0, filterOffset * sizeof(float));
        memcpy(inputTemp + filterOffset, input, inLength * sizeof(float));
        memset(inputTemp + filterOffset + inLength, 0, (filterLength - filterOffset) * sizeof(float));
        
        // Resample
        
        for (unsigned long i = 0, currentOffset = 0; i < nsamps; currentOffset += num)
        {
            currentFilter = tempFilters;

            for (unsigned long j = 0; i < nsamps && j < denom; i++, j++, currentFilter += filterLength)
            {
                long inputOffset = currentOffset + (j * num / denom);
                
    #ifdef TARGET_INTEL
                output[i] = applyFilterVector((vDouble *)currentFilter, inputTemp + inputOffset, filterLength);
    #else
                output[i] = applyFilterScalar(currentFilter, inputTemp + inputOffset, filterLength);
    #endif
            }
        }
        
        // Set not in use free temp memory and return
        
        ALIGNED_FREE(inputTemp);
        ALIGNED_FREE(tempFilters);

        return output;
    }

    void rateAsRatio(double rate, unsigned long& numerator, unsigned long& denominator)
    {
        long Cf[256];
        
        double npart = fabs(rate);
        double ipart;
        
        long num = 1;
        long denom = 1;
        long swap;
        long length = 0;
        long i, j;
        
        for (; npart < 1000 && npart > 0 && length < 256; length++)
        {
            ipart = floor(npart);
            npart = npart - ipart;
            npart = npart ? 1 / npart : 0;
            
            Cf[length] = ipart;
        }
            
        for (i = length - 1; i >= 0; i--)
        {
            num = Cf[i];
            denom = 1;
                
            for (j = i - 1; j >= 0 && denom < 1000; j--)
            {
                swap = num;
                num = denom;
                denom = swap;
                num = num + (denom * Cf[j]);
            }
            
            if (denom < 1000)
                break;
        }
                
        numerator = num;
        denominator = denom;
    }

    float *copyInput(float *input, unsigned long inLength, unsigned long& outLength)
    {
        float *output = new float[inLength];
        
        memcpy(output, input, sizeof(float) * inLength);
        
        outLength = inLength;
        
        return output;
    }
    
public:

    float *process(float *input, unsigned long inLength, unsigned long& outLength, double inSR, double outSR, double transposeRatio = 1.0)
    {
        unsigned long numerator, denominator;
        
        // Calculate overall rate change
        
        double rate = fabs(transposeRatio * inSR / outSR);
    
        // Resample
        
        rateAsRatio(rate, numerator, denominator);
        
        if (numerator == 1 && denominator == 1)
            return copyInput(input, inLength, outLength);
        
        outLength = ceil(((double) denominator * (double) inLength) / (double) numerator);
        
        return resampleRatio(input, inLength, outLength, numerator, denominator);
    }
    
private:
    
    double *mFilter;
    long mNumZeros;
    long mNumPoints;
};


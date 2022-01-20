

#ifndef __HISSTOOLS_SPECTRAL_DISPLAY__
#define __HISSTOOLS_SPECTRAL_DISPLAY__


#include "HISSTools_Controls.hpp"
#include "Spectral_Plot/Spectrum_Plot.h"


class HISSTools_Spectral_Curve_Export : public HISSTools::SpectralCurve<HISSTools_Color_Spec *, HISSTools_VecLib>
{

public:
	
    HISSTools_Spectral_Curve_Export(HISSTools_VecLib *vecDraw, double x, double y, double w, double h, double thickness, HISSTools_Color_Spec *colorSpec, unsigned long maxFFTSize) : HISSTools::SpectralCurve<HISSTools_Color_Spec *, HISSTools_VecLib>(0, 0, w, h, thickness, colorSpec, maxFFTSize)
	{}
	
	~HISSTools_Spectral_Curve_Export()
	{}
		
public:
    // FIX - LICE only
    /*
	void exportBitmap(HISSTools_VecLib *vecDraw, LICE_IBitmap *destinationBitmap, double x, double y, double w, double h, double freqMin, double freqMax, double dbMin, double dbMax, double subSampleRender)
	{		
        HISSTools::SpectralCurve<HISSTools_Color_Spec *, HISSTools_VecLib> temp = HISSTools::SpectralCurve<HISSTools_Color_Spec *, HISSTools_VecLib>(x, y, w, h, mCurveTK, mCurveCS, mSpectrum->getFFTSize());
				
		temp.inputSpectrum(mSpectrum);
        temp.setDisplay(true);
		temp.draw(vecDraw, freqMin, freqMax, dbMin, dbMax, subSampleRender);
		
		return;
	}
     */
};


class HISSTools_Spectral_Display : public HISSTools::SpectrumPlot<HISSTools_Color_Spec *, HISSTools_VecLib>,
public iplug::igraphics::IControl, public HISSTools_Control_Layers
{

public:
    
	HISSTools_Spectral_Display(double x, double y, double w, double h, unsigned long maxFFTSize, int numCurves = 1, const char *type = "", HISSTools_Design_Scheme *designScheme = &DefaultDesignScheme)
	: HISSTools::SpectrumPlot<HISSTools_Color_Spec *, HISSTools_VecLib>(x, y, w, h), IControl( IRECT()), HISSTools_Control_Layers()
	{
		mFreqMin = designScheme->getDimension("SpectralDisplayFreqMin", type);
		mFreqMax = designScheme->getDimension("SpectralDisplayFreqMax", type);
		mDBMin = designScheme->getDimension("SpectralDisplayDbMin", type);
		mDBMax = designScheme->getDimension("SpectralDisplayDbMax", type);

        setRangeX(mFreqMin, mFreqMax, true);
        setRangeY(mDBMin, mDBMax, false);
        
		mFrameCS = designScheme->getColorSpec("SpectralDisplayFrame", type);
		mGridCS = designScheme->getColorSpec("SpectralDisplayGrid", type);
		mTickCS = designScheme->getColorSpec("SpectralDisplayTick", type);
		mBackgroundCS = designScheme->getColorSpec("SpectralDisplayBackground", type);
		
		double gridOctaveSpacing = designScheme->getDimension("SpectralDisplayGridOctaveSpacing", type);
		double gridFreqReference = designScheme->getDimension("SpectralDisplayGridFreqReference", type);
		double gridDbSpacing =  designScheme->getDimension("SpectralDisplayGridDbSpacing", type);
		double gridDbReference =  designScheme->getDimension("SpectralDisplayGridDbReference", type);
	
        setGridX(gridFreqReference, pow(2.0, gridOctaveSpacing), true);
        setGridY(gridDbReference, gridDbSpacing, false);

        mFrameTK = designScheme->getDimension("SpectralDisplayFrame", type);
        mGridTK = designScheme->getDimension("SpectralDisplayGrid", type);
		mTickTK = designScheme->getDimension("SpectralDisplayTick", type);

		mSubSampleRender = designScheme->getDimension("SpectralCurveSubSample");
		
		numCurves = numCurves < 1 ? 1 : numCurves;
		
		for (int i = 0; i < numCurves; i++)
		{
			char curveName[32];
			
			sprintf(curveName, "SpectralCurve%d", i + 1);
			
			double curveTK = designScheme->getDimension(curveName, type);
			HISSTools_Color_Spec *curveCS = designScheme->getColorSpec(curveName, type);
			
			curveTK = (curveTK == 0.0) ? designScheme->getDimension("SpectralCurve", type) : curveTK;
			curveCS = (curveCS == NULL) ? designScheme->getColorSpec("SpectralCurve", type) : curveCS;
			
            addCurve(curveTK, curveCS, maxFFTSize);
		}
		
		// Bounds
		
		HISSTools_Bounds fullBounds(x, y, w, h);
        fullBounds.addThickness(mFrameTK);
        
        mRECT = fullBounds;
        mTargetRECT = mRECT;
	}
		
	~HISSTools_Spectral_Display()
	{		
		//for (std::vector<HISSTools_Spectral_Curve *>::iteratorit = mCurves.begin(); it != mCurves.end(); it++)
		//	delete (*it);
	}

public:
    // FIX - LICE only
    /*
	bool exportBitmap(char *path, double w, double h, double pad)
	{
		LICE_MemBitmap bitmap(w + 2 * pad, h + 2 * pad);
        HISSTools_VecLib drawObject(&bitmap, 8);
        
        // FIX - find a better way of doing this!
		// FIX - all size parameters must not be read from object...
        
		mDrawX = pad;
		mDrawY = pad;
		mDrawW = w;
		mDrawH = h;

        // Clear / draw background / draw curves / write file
        
        LICE_Clear(&bitmap, LICE_RGBA(0, 0, 0, 0));
        
		DrawBackground(&drawObject);
		
        for (std::vector<HISSTools_Spectral_Curve *>::iterator it = mCurves.begin(); it != mCurves.end(); it++)
			(*it)->exportBitmap(&drawObject, &bitmap, pad, pad, w, h, mFreqMin, mFreqMax, mDbMin, mDbMax, mSubSampleRender);
		
        return true;//LICE_WritePNG(path, &bitmap);
	}
	*/
public:
	
    void Draw(IGraphics& graphics) override
	{	
        HISSTools_VecLib vecDraw(graphics);

        draw(vecDraw);		
	}
	
    void needsRedraw() override
    {
        SetDirty(true);
    }

    void OnMouseDblClick(float x, float y, const IMouseMod& mod) override
    {
        setRangeX(mFreqMin, mFreqMax, true);
        setRangeY(mDBMin, mDBMax, false);
    }
    
    void OnMouseWheel(float x, float y, const IMouseMod& mod, float d) override
    {
        // FIX old iplug features?
        //x /= mVecDraw->getScaling();
        //y /= mVecDraw->getScaling();
               
        double gearing = pow(1.2, d);

        if (!mod.S)
        {
            double freqLo = std::max(mFreqMin, getScaling().posToXVal(x - ((x - getScaling().getL()) * gearing)));
            double freqHi = std::min(mFreqMax, getScaling().posToXVal(x - ((x - getScaling().getR()) * gearing)));
        
            limitZoom(freqLo, freqHi, getScaling().posToXVal(x), 1.0, true);
            setRangeX(freqLo, freqHi, true);
        }
        
        if (!mod.C)
        {
            double dbLo = std::max(mDBMin, getScaling().posToYVal(y + ((getScaling().getB() - y) * gearing)));
            double dbHi = std::min(mDBMax, getScaling().posToYVal(y + ((getScaling().getT() - y) * gearing)));
        
            limitZoom(dbLo, dbHi, getScaling().posToYVal(y), 20.0, false);
            setRangeY(dbLo, dbHi, false);
        }
    }
    
    void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override
    {
        double freqLo, freqHi, dbLo, dbHi;
        
        // Calculate
        
        x = -dX; // mVecDraw->getScaling();
        y = -dY; // mVecDraw->getScaling();
        
        // Do X dragging
        
        if (x >= 0)
        {
            freqHi = std::min(mFreqMax, getScaling().posToXVal(getScaling().getR() + x));
            freqLo = getScaling().posToXVal(getScaling().xValToPos(freqHi) - getScaling().getWidth());
        }
        else
        {
            freqLo = std::max(mFreqMin, getScaling().posToXVal(getScaling().getL() + x));
            freqHi = getScaling().posToXVal(getScaling().xValToPos(freqLo) + getScaling().getWidth());
        }
        
        setRangeX(freqLo, freqHi, true);
        
        // Do Y dragging
        
        if (y <= 0)
        {
            dbHi = std::min(mDBMax, getScaling().posToYVal(getScaling().getT() + y));
            dbLo = getScaling().posToYVal(getScaling().yValToPos(dbHi) + getScaling().getHeight());
        }
        else
        {
            dbLo = std::max(mDBMin, getScaling().posToYVal(getScaling().getB() + y));
            dbHi = getScaling().posToYVal(getScaling().yValToPos(dbLo) - getScaling().getHeight());
        }
        
        // Set ranges
        
        setRangeY(dbLo, dbHi, false);
    }
    
    void SetRanges(double freqMin, double freqMax, double dbMin, double dbMax)
    {
        mFreqMin = freqMin;
        mFreqMax = freqMax;
        mDBMin = dbMin;
        mDBMax = dbMax;
        
        setRangeX(mFreqMin, mFreqMax, true);
        setRangeY(mDBMin, mDBMax, false);
    }
    
private:
	
    void limitZoom(double &lo, double &hi, double pivot, double limit, bool logarithmic)
    {
        double loI = logarithmic ? log2(lo) : lo;
        double hiI = logarithmic ? log2(hi) : hi;
        
        if ((hiI - loI) < limit)
        {
            pivot = logarithmic ? log2(pivot) : pivot;
            
            double ratio = (pivot - loI) / (hiI - loI);
        
            lo = pivot - ratio * limit;
            hi = pivot + (1.0 - ratio) * limit;
            
            if (logarithmic)
            {
                lo = exp2(lo);
                hi = exp2(hi);
            }
        }
    }
    
	// Drawing Object
	    
    double mDrawingScale;
    double mFreqMin, mFreqMax;
    double mDBMin, mDBMax;
};


#endif

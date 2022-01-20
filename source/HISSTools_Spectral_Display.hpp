

#ifndef __HISSTOOLS_SPECTRAL_DISPLAY__
#define __HISSTOOLS_SPECTRAL_DISPLAY__


#include <HISSTools_UI/HISSTools_Controls.hpp>
#include "Spectrum_Plot.h"


class HISSTools_Spectral_Curve_Export : public HISSTools::SpectralCurve<HISSTools_Color_Spec *, HISSTools_LICE_Vec_Lib>
{

public:
	
    HISSTools_Spectral_Curve_Export(HISSTools_LICE_Vec_Lib *vecDraw, double x, double y, double w, double h, double thickness, HISSTools_Color_Spec *colorSpec, unsigned long maxFFTSize) : HISSTools::SpectralCurve<HISSTools_Color_Spec *, HISSTools_LICE_Vec_Lib>(0, 0, w, h, thickness, colorSpec, maxFFTSize)
	{}
	
	~HISSTools_Spectral_Curve_Export()
	{}
		
public:
		
	void exportBitmap(HISSTools_LICE_Vec_Lib *vecDraw, LICE_IBitmap *destinationBitmap, double x, double y, double w, double h, double freqMin, double freqMax, double dbMin, double dbMax, double subSampleRender)
	{		
        HISSTools::SpectralCurve<HISSTools_Color_Spec *, HISSTools_LICE_Vec_Lib> temp = HISSTools::SpectralCurve<HISSTools_Color_Spec *, HISSTools_LICE_Vec_Lib>(x, y, w, h, mCurveTK, mCurveCS, mSpectrum->getFFTSize());
				
		temp.inputSpectrum(mSpectrum);
        temp.setDisplay(true);
		temp.draw(vecDraw, freqMin, freqMax, dbMin, dbMax, subSampleRender);
		
		return;
	}
};


class HISSTools_Spectral_Display : public HISSTools::SpectrumPlot<HISSTools_Color_Spec *, HISSTools_LICE_Vec_Lib>,
    public IControl, public HISSTools_Control_Layers
{

public:
    
	HISSTools_Spectral_Display(IPlugBase* pPlug, HISSTools_LICE_Vec_Lib *vecDraw, double x, double y, double w, double h, unsigned long maxFFTSize, int numCurves = 1, const char *type = "", HISSTools_Design_Scheme *designScheme = &DefaultDesignScheme)
	: HISSTools::SpectrumPlot<HISSTools_Color_Spec *, HISSTools_LICE_Vec_Lib>(x, y, w, h), IControl(pPlug, IRECT()), HISSTools_Control_Layers()
	{
		mVecDraw = vecDraw;
        
		mDefFreqMin = mFreqMin = designScheme->getDimension("SpectralDisplayFreqMin", type);
		mDefFreqMax = mFreqMax = designScheme->getDimension("SpectralDisplayFreqMax", type);
		mDefDBMin = mDBMin = designScheme->getDimension("SpectralDisplayDbMin", type);
		mDefDBMax = mDBMax = designScheme->getDimension("SpectralDisplayDbMax", type);

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
		mRECT = fullBounds.iBounds(vecDraw->getScaling());
        mTargetRECT = mRECT;
	}
		
	~HISSTools_Spectral_Display()
	{		
		//for (std::vector<HISSTools_Spectral_Curve *>::iteratorit = mCurves.begin(); it != mCurves.end(); it++)
		//	delete (*it);
	}

public:
    
	bool exportBitmap(char *path, double w, double h, double pad)
	{
		LICE_MemBitmap bitmap(w + 2 * pad, h + 2 * pad);
        HISSTools_LICE_Vec_Lib drawObject(&bitmap, 8);
        
        // FIX - find a better way of doing this!
		// FIX - all size parameters must not be read from object...
        /*
		mDrawX = pad;
		mDrawY = pad;
		mDrawW = w;
		mDrawH = h;*/

        // Clear / draw background / draw curves / write file
        
        LICE_Clear(&bitmap, LICE_RGBA(0, 0, 0, 0));
        /*
		DrawBackground(&drawObject);
		
        for (std::vector<HISSTools_Spectral_Curve *>::iterator it = mCurves.begin(); it != mCurves.end(); it++)
			(*it)->exportBitmap(&drawObject, &bitmap, pad, pad, w, h, mFreqMin, mFreqMax, mDbMin, mDbMax, mSubSampleRender);
		*/
        return true;//LICE_WritePNG(path, &bitmap);
	}
	
public:
	
	virtual bool Draw(IGraphics* pGraphics)
	{	
		mVecDraw->setBitmap(pGraphics->GetDrawBitmap());
        mVecDraw->setClip(pGraphics->GetDrawRECT().Intersect(&mRECT));
		
        draw(mVecDraw);
		
		return true;
	}
	
    void needsRedraw()
    {
        SetDirty(true);
    }

    virtual void OnMouseDblClick(int x, int y, IMouseMod* pMod)
    {
        mFreqMin = mDefFreqMin;
        mFreqMax = mDefFreqMax;
        mDBMin = mDefDBMin;
        mDBMax = mDefDBMax;

        setRangeX(mDefFreqMin, mDefFreqMax, true);
        setRangeY(mDefDBMin, mDefDBMax, false);
    }
    
    virtual void OnMouseWheel(int x, int y, IMouseMod* pMod, int d)
    {
        x /= mVecDraw->getScaling();
        y /= mVecDraw->getScaling();
       
        // FIX - clipping when too zoomed...
        
        double gearing = pow(1.2, ConvertMouseDeltaToNative(d));

        mFreqMin = getScaling()->posToXVal(x - (x - getScaling()->getL()) * gearing);
        mFreqMax = getScaling()->posToXVal(x - (x - getScaling()->getR()) * gearing);
        
        mDBMin = getScaling()->posToYVal(y + (getScaling()->getB() - y) * gearing);
        mDBMax = getScaling()->posToYVal(y + (getScaling()->getT() - y) * gearing);
        
        mFreqMin = std::max(mFreqMin, mDefFreqMin);
        mFreqMax = std::min(mFreqMax, mDefFreqMax);
        
        mDBMin = std::max(mDBMin, mDefDBMin);
        mDBMax = std::min(mDBMax, mDefDBMax);
        
        setRangeX(mFreqMin, mFreqMax, true);
        setRangeY(mDBMin, mDBMax, false);
    }
    
    virtual void OnMouseDrag(int x, int y, int dX, int dY, IMouseMod* pMod)
    {
        // FIX - Hi-res deltas in WDL

        // Calculate
        
        x = ConvertMouseDeltaToNative(-dX) / mVecDraw->getScaling();
        y = ConvertMouseDeltaToNative(-dY) / mVecDraw->getScaling();
        
        double width = getScaling()->getWidth();
        
        if (x >= 0)
        {
            mFreqMax = getScaling()->posToXVal(getScaling()->getR() + x);
            mFreqMax = std::min(mFreqMax, mDefFreqMax);
            mFreqMin = getScaling()->posToXVal(getScaling()->xValToPos(mFreqMax) - width);
        }
        else
        {
            mFreqMin = getScaling()->posToXVal(getScaling()->getL() + x);
            mFreqMin = std::max(mFreqMin, mDefFreqMin);
            mFreqMax = getScaling()->posToXVal(getScaling()->xValToPos(mFreqMin) + width);
        }
        
        double height = getScaling()->getHeight();
        
        if (y <= 0)
        {
            mDBMax = getScaling()->posToYVal(getScaling()->getT() + y);
            mDBMax = std::min(mDBMax, mDefDBMax);
            mDBMin = getScaling()->posToYVal(getScaling()->yValToPos(mDBMax) + height);
        }
        else
        {
            mDBMin = getScaling()->posToYVal(getScaling()->getB() + y);
            mDBMin = std::max(mDBMin, mDefDBMin);
            mDBMax = getScaling()->posToYVal(getScaling()->yValToPos(mDBMin) - height);
        }
        
        // Set ranges
        
        setRangeX(mFreqMin, mFreqMax, true);
        setRangeY(mDBMin, mDBMax, false);
    }
    
private:
	
	// Drawing Object
	
	HISSTools_LICE_Vec_Lib *mVecDraw;
    
    double mDrawingScale;
    double mDefFreqMin, mDefFreqMax, mFreqMin, mFreqMax;
    double mDefDBMin, mDefDBMax, mDBMin, mDBMax;
};


#endif
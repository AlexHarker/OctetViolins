

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
        
		double freqMin = designScheme->getDimension("SpectralDisplayFreqMin", type);
		double freqMax = designScheme->getDimension("SpectralDisplayFreqMax", type);
		double dbMin = designScheme->getDimension("SpectralDisplayDbMin", type);
		double dbMax = designScheme->getDimension("SpectralDisplayDbMax", type);

        setRangeX(freqMin, freqMax, true);
        setRangeY(dbMin, dbMax, false);
        
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
	
	bool Draw(IGraphics* pGraphics)
	{	
		mVecDraw->setBitmap(pGraphics->GetDrawBitmap());
		cachedBackground(mVecDraw, mRECT);
		
        draw(mVecDraw);
		
		return true;
	}
	
    void needsRedraw()
    {
        SetDirty(true);
    }
    
private:
	
	// Drawing Object
	
	HISSTools_LICE_Vec_Lib *mVecDraw;
};


#endif
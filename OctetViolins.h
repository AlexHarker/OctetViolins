#ifndef __IPLUGEFFECT__
#define __IPLUGEFFECT__

#include "IPlug_include_in_plug_hdr.h"

#include <HISSTools_UI/HISSTools_Controls.hpp>
#include <HISSTools_UI/HISSTools_Design_Scheme.hpp>

#include "HISSTools_Spectral_Display.hpp"

#include "HISSTools_Utility/HISSTools_ThreadSafety.hpp"

#include "HISSTools_FFT.hpp"
#include "Spectrum.hpp"
#include "PowerSpectrum.hpp"

#include "CrossfadedConvolution.h"

//#include "OctetViolins_Helper_Classes.hpp"

#define GUI_WIDTH 760
#define GUI_HEIGHT 680

enum EParams
{
	kNumIRs,
	kIRVisible,
	
	kIR1,
	kIRVolume1,
	kIRTransposition1,
	kIRHPFFreq1,
	kIRLPFFreq1,
	kIRHPFOn1,
	kIRLPFOn1,
	kIRMute1,
	kIRSolo1,
	
	kIR2,
	kIRVolume2,
	kIRTransposition2,
	kIRHPFFreq2,
	kIRLPFFreq2,
	kIRHPFOn2,
	kIRLPFOn2,
	kIRMute2,
	kIRSolo2,
	
	kIR3,
	kIRVolume3,
	kIRTransposition3,
	kIRHPFFreq3,
	kIRLPFFreq3,
	kIRHPFOn3,
	kIRLPFOn3,
	kIRMute3,
	kIRSolo3,
	
	kSource1,
	kSource2,
	kSource3,
	kSource4,
	
	kDelay1,
	kDelay2,
	kDelay3,
	kDelay4,
	
	kIRSelect1,
	kIRSelect2,
	kIRSelect3,
	
	kNumParams
};


static inline int numIRParams() { return kIR2 - kIR1; }


class OctetViolins : public IPlug
{
	
	enum MeasurementMode {kHarmonics, kEQ, kTone, kCalibration};
	
public:
	
	OctetViolins(IPlugInstanceInfo instanceInfo);
	~OctetViolins();
	
	void CreateControls(IGraphics *pGraphics);
	void OnWindowResize();
	
	void ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames);
	void Reset();
	
	void OnParamChange(int paramIdx, ParamChangeSource source);
	void LoadUntilUpdated();
	
	void AddIR();
	void RemoveIR();
	
private:
	
	void CheckVisibleIR();
	void SetIRDisplay(int i, bool setParam);
	long DelayInSamps(int i);
	bool GetParamUpdated();
	bool GetSoloChanged();
	void LoadFiles(int diff, HISSTools_RefPtr<double> &IRL, HISSTools_RefPtr<double> &IRR);
	void LoadIRs();
	void MixIRs();
	void SetChanged(int i, bool state);
	void UpdateIRsAndDisplay(bool displayOnly = false);
	void DisplaySpectrum(HISSTools_RefPtr <double> IR, unsigned long index, double samplingRate);
	void FadeIR(HISSTools_RefPtr <double> ir,  uintptr_t fadeIn,  uintptr_t fadeOut);
	
	void ClearParamCache();
	bool UpdateParamCache(int start = 0, int end = kNumParams);
	
	void UpdateControlAndParam(double value, IControl *control, bool paramChange = false);

	// Controls and Display
	
	HISSTools_Spectral_Display *mSpectralDisplay;
	
	IControl *mAddIRButtons[2];
	IControl *mRemoveIRButton;
	
	HISSTools_Value *mSelections[3];
	HISSTools_Dial *mAmps[3];
	HISSTools_Dial *mTranspositions[3];
	HISSTools_Dial *mHPFFreqs[3];
	HISSTools_Dial *mLPFFreqs[3];
	HISSTools_Button *mHPFSwitches[3];
	HISSTools_Button *mLPFSwitches[3];
	HISSTools_Button *mMuteSwitches[3];
	HISSTools_Button *mSoloSwitches[3];
	HISSTools_Button *mSelectSwitches[3];
	HISSTools_Invisible_Tabs *mIRTab;
	
	// DSP
	
	CrossfadedConvolution mConvolver;
	
	// Drawing
	
	HISSTools_LICE_Vec_Lib mVecDraw;	
	
	// IR Storage
	
	HISSTools_RefPtr <double> mIRs[4][2];
	
	// Default IR Setup
	
	double mIRDefaultValues[kIR2 - kIR1];
	
	// Temporary / Working Parameters
	
	double mSamplingRate;
	
	double mParamCache[kNumParams];
	
	HANDLE mThread;
	
	int mChanged[4];
	
	bool mSolo, mSoloChanged, mParamUpdated, mUpdateAudioEngine;
};

class AddIRButton : public HISSTools_Button
{
	
public:
	
	AddIRButton(OctetViolins *pPlug, HISSTools_LICE_Vec_Lib *vecDraw, double x, double y, double w = 0, double h = 0, const char *type = 0, HISSTools_Design_Scheme *designScheme = &DefaultDesignScheme) : mPlug(pPlug), HISSTools_Button(mPlug, -1, vecDraw, x, y, w, h, type, designScheme)
	{
		mName = "+";
	}
	
	void OnMouseDown(int x, int y, IMouseMod* pMod)
	{
		mValue = 1.0;
		SetDirty(false);
	}
	
	void OnMouseUp(int x, int y, IMouseMod* pMod)
	{
		mValue = 0.0;
		SetDirty(false);
		mPlug->AddIR();
	}
	
private:
	
	OctetViolins *mPlug;
};

class RemoveIRButton : public HISSTools_Button
{
	
public:
	
	RemoveIRButton(OctetViolins *pPlug, HISSTools_LICE_Vec_Lib *vecDraw, double x, double y, double w = 0, double h = 0, HISSTools_Design_Scheme *designScheme = &DefaultDesignScheme) : mPlug(pPlug), HISSTools_Button(mPlug, -1, vecDraw, x, y, w, h, "remove", designScheme)
	{
		mName = "Delete";
	}
	
	void OnMouseDown(int x, int y, IMouseMod* pMod)
	{
		mValue = 1.0;
		SetDirty(false);
	}
	
	void OnMouseDrag(int x, int y, int dX, int dY, IMouseMod* pMod)
	{
		mValue = mTargetRECT.Contains(x, y);
		SetDirty(false);
	}
	
	void OnMouseUp(int x, int y, IMouseMod* pMod)
	{
		OnMouseDrag(x, y, 0, 0, pMod);

		if (mValue)
			mPlug->RemoveIR();
		
		mValue = 0.0;
		SetDirty();
	}
	
private:
	
	OctetViolins *mPlug;
};

#endif

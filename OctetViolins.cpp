
#include "IControl.h"

#include "OctetViolins.h"
#include "IPlug_include_in_plug_src.h"

#include "resource.h"
#include <math.h>

#include "IAudioFile.h"
#include "Resampler.h"
#include "Biquad.h"

const int kNumPrograms = 1;

DWORD LoadingThread(LPVOID plugParam)
{
	OctetViolins *plug = (OctetViolins *) plugParam;
	
	plug->LoadUntilUpdated();
	
	return NULL;
}

const char *paths[4][4] =
{
	{
		"IRs/Accelerometer_01_Treble.wav",
		"IRs/Accelerometer_02_Soprano.wav",
		"IRs/Accelerometer_03_Mezzo.wav",
		"IRs/Accelerometer_04_Alto.wav",
	},
	{
		"IRs/Neumann_01_Treble.wav",
		"IRs/Neumann_02_Soprano.wav",
		"IRs/Neumann_03_Mezzo.wav",
		"IRs/Neumann_04_Alto.wav"
	},
	{
		"IRs/Neumann_Pair_01_Treble.wav",
		"IRs/Neumann_Pair_02_Soprano.wav",
		"IRs/Neumann_Pair_03_Mezzo.wav",
		"IRs/Neumann_Pair_04_Alto.wav",
	},
	{
		"IRs/Gras_Pair_01_Treble.wav",
		"IRs/Gras_Pair_02_Soprano.wav",
		"IRs/Gras_Pair_03_Mezzo.wav",
		"IRs/Gras_Pair_04_Alto.wav",
	}
};

OctetViolins::OctetViolins(IPlugInstanceInfo instanceInfo)
:	IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo), mSamplingRate(44100.), mThread(NULL), mUpdateAudioEngine(false), mParamUpdated(false)
{
	TRACE;

	// Define Parameters
	
	GetParam(kNumIRs)->InitInt("Number of IRs", 1, 1, 3, "");
	
	GetParam(kIRVisible)->InitInt("Selected IR", 1, 1, 3, "");
	GetParam(kIRVisible)->SetCanAutomate(false);
	
	for (int i = 0; i < 3; i++)
	{
		// FIX - better naming
		
		int diff = i * (kIR2 - kIR1);
		
		GetParam(kIR1 + diff)->InitEnum("Instrument", 0, 4);
		GetParam(kIR1 + diff)->SetDisplayText(0, "Treble");
		GetParam(kIR1 + diff)->SetDisplayText(1, "Soprano");
		GetParam(kIR1 + diff)->SetDisplayText(2, "Mezzo");
		GetParam(kIR1 + diff)->SetDisplayText(3, "Alto");
	
		GetParam(kIRVolume1 + diff)->InitDouble("Amp", 0.0, -40, 30, 0.1, "dB");
		GetParam(kIRTransposition1 + diff)->InitDouble("Transposition", 0, -12, 12, 0.1, "st");
		GetParam(kIRHPFFreq1 + diff)->InitDouble("HPF Freq", 1000, 10, 20000, 1.0, "Hz", "", 2.0);
		GetParam(kIRLPFFreq1 + diff)->InitDouble("LPF Freq", 1000, 10, 20000, 1.0, "Hz", "", 2.0);

		GetParam(kIRHPFOn1 + diff)->InitEnum("HPF On", 0, 2);
		GetParam(kIRHPFOn1 + diff)->SetDisplayText(0, "Off");
		GetParam(kIRHPFOn1 + diff)->SetDisplayText(1, "On");

		GetParam(kIRLPFOn1 + diff)->InitEnum("LPF On", 0, 2);
		GetParam(kIRLPFOn1 + diff)->SetDisplayText(0, "Off");
		GetParam(kIRLPFOn1 + diff)->SetDisplayText(1, "On");
		
		GetParam(kIRMute1 + diff)->InitEnum("Mute", 0, 2);
		GetParam(kIRMute1 + diff)->SetDisplayText(0, "Off");
		GetParam(kIRMute1 + diff)->SetDisplayText(1, "On");
		
		GetParam(kIRSolo1 + diff)->InitEnum("Solo", 0, 2);
		GetParam(kIRSolo1 + diff)->SetDisplayText(0, "Off");
		GetParam(kIRSolo1 + diff)->SetDisplayText(1, "On");
	}
	
	GetParam(kSource1)->InitDouble("Accel", 0.0, 0.0, 1.0, 0.01, "", "", 2.0);
	GetParam(kSource2)->InitDouble("Cardioid", 1.0, 0.0, 1.0, 0.01, "", "", 2.0);
	GetParam(kSource3)->InitDouble("Pair 1", 0.0, 0.0, 1.0, 0.01, "", "", 2.0);
	GetParam(kSource4)->InitDouble("Pair 2", 0.0, 0.0, 1.0, 0.01, "", "", 2.0);
	
	GetParam(kDelay1)->InitDouble("Delay", 0.0, 0.0, 20.0, 0.1, "ms");
	GetParam(kDelay2)->InitDouble("Delay", 0.0, 0.0, 20.0, 0.1, "ms");
	GetParam(kDelay3)->InitDouble("Delay", 0.0, 0.0, 20.0, 0.1, "ms");
	GetParam(kDelay4)->InitDouble("Delay", 0.0, 0.0, 20.0, 0.1, "ms");
	
	GetParam(kIRSelect1)->InitBool("IR 1", true);
	GetParam(kIRSelect2)->InitBool("IR 2", false);
	GetParam(kIRSelect3)->InitBool("IR 3", false);
	
	GetParam(kIRSelect1)->SetCanAutomate(false);
	GetParam(kIRSelect2)->SetCanAutomate(false);
	GetParam(kIRSelect3)->SetCanAutomate(false);
	
	MakeDefaultPreset("-", kNumPrograms);
	
	IGraphics* pGraphics = MakeGraphics(this, GUI_WIDTH, GUI_HEIGHT);

	CreateControls(pGraphics);
	AttachGraphics(pGraphics);
	
	ClearParamCache();
	OnParamReset(kReset);
};

OctetViolins::~OctetViolins()
{
}

class Design : public HISSTools_Design_Scheme
{
	
public:
	
	Design() : HISSTools_Design_Scheme(true)
	{
		// FIX - why goes to default?
		
		addColorSpec("SpectralDisplayBackground", new HISSTools_Color_Spec(1.0, 1.0, 1.0, 1.0));
		
		addDimension("SpectralCurve1", 1.2);
		addDimension("SpectralCurve2", 1.2);
		addDimension("SpectralCurve3", 1.2);
		addDimension("SpectralCurve4", 2.5);
		
		HISSTools_Color_Spec *col1 = new HISSTools_Color_Spec(0.7, 0.0, 0.0, 0.9);
		HISSTools_Color_Spec *col2 = new HISSTools_Color_Spec(0.2, 0.7, 0.3, 0.9);
		HISSTools_Color_Spec *col3 = new HISSTools_Color_Spec(0.1, 0.4, 0.7, 0.9);
		HISSTools_Color_Spec *col4 = new HISSTools_Color_Spec(1.0, 1.0, 1.0, 0.7);
		
		addColorSpec("SpectralCurve1", col1);
		addColorSpec("SpectralCurve2", col2);
		addColorSpec("SpectralCurve3", col3);
		addColorSpec("SpectralCurve4", col4);
		
		addColorSpec("ButtonHandleOn", "1", col1);
		addColorSpec("ButtonHandleOn", "2", col2);
		addColorSpec("ButtonHandleOn", "3", col3);
		
		addColorSpec("DialIndicator", "1",  col1);
		addColorSpec("DialIndicator", "2", col2);
		addColorSpec("DialIndicator", "3", col3);
	}
};

Design theDesign;

void OctetViolins::CreateControls(IGraphics *pGraphics)
{
	IColor bgrb = IColor(255, 140, 140, 140);
				  
	int freqDispX = 30;
	int freqDispY = 30;
	int dispWidth = 700;
	int dispHeight = 300;

	// Setup Drawing
	
	mVecDraw.setBitmap(pGraphics->GetDrawBitmap());
	mVecDraw.setScaling(pGraphics->GetScalingFactor());
	
	// Main Displays

	mSpectralDisplay = new HISSTools_Spectral_Display(this, &mVecDraw, freqDispX, freqDispY, dispWidth, dispHeight, 1050000, 4, 0, &theDesign);
	
	mSpectralDisplay->setRangeX(20.0, 22050., true);
	mSpectralDisplay->setRangeY(-80.0, 20.0, false);
	
	pGraphics->AttachControl(mSpectralDisplay);
	
	// Num IRs and Selection
	
	mSelectSwitches[0] = new HISSTools_Button(this, kIRSelect1, &mVecDraw, freqDispX, dispHeight + 50, 70, 25, "1", &theDesign);
	mSelectSwitches[1] = new HISSTools_Button(this, kIRSelect2, &mVecDraw, freqDispX + 80, dispHeight + 50, 70, 25, "2", &theDesign);
	mSelectSwitches[2] = new HISSTools_Button(this, kIRSelect3, &mVecDraw, freqDispX + 160, dispHeight + 50, 70, 25, "3", &theDesign);
	
	pGraphics->AttachControl(mSelectSwitches[0]);
	pGraphics->AttachControl(mSelectSwitches[1]);
	pGraphics->AttachControl(mSelectSwitches[2]);
	
	//	new HISSTools_Button
	pGraphics->AttachControl(new HISSTools_Value(this, kNumIRs, &mVecDraw, freqDispX + dispWidth - 90, freqDispY + dispHeight + 40, 90, 20));
	//pGraphics->AttachControl(new HISSTools_Value(this, kIRVisible, &mVecDraw, freqDispX + dispWidth - 90, freqDispY + dispHeight + 90, 90, 20));
	
	// FIX - HACK
	
	dispHeight += 70;

	pGraphics->AttachControl(new HISSTools_Value(this, kSource1, &mVecDraw, freqDispX + 200, freqDispY + dispHeight + 20, 60, 20));
	pGraphics->AttachControl(new HISSTools_Value(this, kSource2, &mVecDraw, freqDispX + 280, freqDispY + dispHeight + 20, 60, 20));
	pGraphics->AttachControl(new HISSTools_Value(this, kSource3, &mVecDraw, freqDispX + 360, freqDispY + dispHeight + 20, 60, 20));
	pGraphics->AttachControl(new HISSTools_Value(this, kSource4, &mVecDraw, freqDispX + 440, freqDispY + dispHeight + 20, 60, 20));

	pGraphics->AttachControl(new HISSTools_Value(this, kDelay1, &mVecDraw, freqDispX + 200, freqDispY + dispHeight + 70, 60, 20));
	pGraphics->AttachControl(new HISSTools_Value(this, kDelay2, &mVecDraw, freqDispX + 280, freqDispY + dispHeight + 70, 60, 20));
	pGraphics->AttachControl(new HISSTools_Value(this, kDelay3, &mVecDraw, freqDispX + 360, freqDispY + dispHeight + 70, 60, 20));
	pGraphics->AttachControl(new HISSTools_Value(this, kDelay4, &mVecDraw, freqDispX + 440, freqDispY + dispHeight + 70, 60, 20));
	
	// IR Parameters
	
	// Invisible Tabs
	
	mIRTab = new HISSTools_Invisible_Tabs(this, kIRVisible);

	for (int i = 0; i < 3; i++)
	{
		int diff = i * (kIR2 - kIR1);
		
		mSelections[i] = new HISSTools_Value(this, kIR1 + diff, &mVecDraw, freqDispX + 10, freqDispY + dispHeight + 60, 150, 30);
	
		char dialStyle1[64];
		char dialStyle2[64];
		char dialStyle3[64];
		
		sprintf(dialStyle1, "%d", i + 1);
		sprintf(dialStyle2, "bipolar %d", i + 1);
		sprintf(dialStyle3, "small %d", i + 1);
	
		mAmps[i] = new HISSTools_Dial(this, kIRVolume1 + diff, &mVecDraw, freqDispX + 10, freqDispY + dispHeight + 130, dialStyle1, &theDesign);
		mTranspositions[i] = new HISSTools_Dial(this, kIRTransposition1 + diff, &mVecDraw, freqDispX + 120, freqDispY + dispHeight + 130, dialStyle2, &theDesign);
		mHPFFreqs[i] = new HISSTools_Dial(this, kIRHPFFreq1 + diff, &mVecDraw, freqDispX + 245, freqDispY + dispHeight + 130, dialStyle3, &theDesign);
		mLPFFreqs[i] = new HISSTools_Dial(this, kIRLPFFreq1 + diff, &mVecDraw, freqDispX + 355, freqDispY + dispHeight + 130, dialStyle3, &theDesign);
		
		mHPFSwitches[i] = new HISSTools_Button(this, kIRHPFOn1 + diff, &mVecDraw, freqDispX + 240, freqDispY + dispHeight + 230, 70, 20);
		mLPFSwitches[i] = new HISSTools_Button(this, kIRLPFOn1 + diff, &mVecDraw, freqDispX + 350, freqDispY + dispHeight + 230, 70, 20);
	
		mMuteSwitches[i] = new HISSTools_Button(this, kIRMute1 + diff, &mVecDraw, freqDispX + 460, freqDispY + dispHeight + 230, 70, 20);
		mSoloSwitches[i] = new HISSTools_Button(this, kIRSolo1 + diff, &mVecDraw, freqDispX + 460, freqDispY + dispHeight + 190, 70, 20);
		
		pGraphics->AttachControl(mSelections[i]);
		pGraphics->AttachControl(mAmps[i]);
		pGraphics->AttachControl(mTranspositions[i]);
		pGraphics->AttachControl(mHPFFreqs[i]);
		pGraphics->AttachControl(mLPFFreqs[i]);
		pGraphics->AttachControl(mHPFSwitches[i]);
		pGraphics->AttachControl(mLPFSwitches[i]);
		pGraphics->AttachControl(mMuteSwitches[i]);
		pGraphics->AttachControl(mSoloSwitches[i]);
		
		mIRTab->attachControl(mSelections[i], i);
		mIRTab->attachControl(mAmps[i], i);
		mIRTab->attachControl(mTranspositions[i], i);
		mIRTab->attachControl(mHPFFreqs[i], i);
		mIRTab->attachControl(mLPFFreqs[i], i);
		mIRTab->attachControl(mHPFSwitches[i], i);
		mIRTab->attachControl(mLPFSwitches[i], i);
		mIRTab->attachControl(mMuteSwitches[i], i);
		mIRTab->attachControl(mSoloSwitches[i], i);
	}
	
	pGraphics->AttachControl(mIRTab);
	pGraphics->AttachPanelBackground(&bgrb);
}

void OctetViolins::OnWindowResize()
{
	CreateControls(GetGUI());
	
	// Set parameters on GUI
	
	for (unsigned int i = 0; i < kNumParams; i++)
		GetGUI()->SetParameterFromPlug(i, GetParam(i)->GetDefaultNormalized(), true);

	// Ensure display parameters are correct in all regards
	
	for (int i = 0; i < 4; i++)
		SetChanged(i, true);
	OnParamReset(kGUI);
	UpdateIRsAndDisplay(true);
	
	// Redraw all the controls
	
	RedrawParamControls();
}

void OctetViolins::ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames)
{
	if (mUpdateAudioEngine && mConvolver.set(mIRs[3][0].get(), mIRs[3][1].get(), mIRs[3][0].getSize()))
		mUpdateAudioEngine = false;
	
	mConvolver.process((const double **) inputs, outputs, nFrames);
}


void OctetViolins::Reset()
{
	TRACE;
	IMutexLock lock(this);
	
	ClearParamCache();
	mSamplingRate = GetSampleRate();

	mConvolver.reset();
}

void OctetViolins::LoadUntilUpdated()
{	
	while (GetParamUpdated())
		LoadIRs();
	
	IMutexLock lock(this);
		
	mThread = NULL;
}

bool OctetViolins::GetParamUpdated()
{
	IMutexLock lock(this);
	
	bool paramUpdated = mParamUpdated;
	mParamUpdated = false;
	
	return paramUpdated;
}


bool OctetViolins::GetSoloChanged()
{
	IMutexLock lock(this);

	bool soloChanged = mSoloChanged;
	mSoloChanged = false;

	return soloChanged;
}

long OctetViolins::DelayInSamps(int i)
{
	return round (GetParam(kDelay1 + i)->Value() * mSamplingRate / 1000.0);
}

void OctetViolins::LoadFiles(int diff, HISSTools_RefPtr<double> &IRL, HISSTools_RefPtr<double> &IRR)
{
	unsigned long outLength;
	
	double outSR = mSamplingRate * pow(2.0, -GetParam(kIRTransposition1 + diff)->Value() / 12.0);
	
	HISSTools_RefPtr<double> filesL[4];
	HISSTools_RefPtr<double> filesR[4];
	
	// Read Files
	
	for (int i = 0; i < 4; i++)
	{
		char fullPath[4096];
		char bundlePath[4096];
		
		CFBundleRef requestedBundle = CFBundleGetBundleWithIdentifier(CFSTR(BUNDLE_ID));
		CFURLRef url = CFBundleCopyBundleURL(requestedBundle);
		CFURLGetFileSystemRepresentation(url, TRUE, (UInt8 *) bundlePath, (CFIndex) 4096);
		
		snprintf(fullPath, 4096, "%s/Contents/Resources/%s", bundlePath, paths[i][GetParam(kIR1 + diff)->Int()]);
		
		HISSTools::IAudioFile file(fullPath);
		
		if (GetParam(kSource1 + i)->Value() && file.isOpen() && !file.getErrorFlags() )
		{
			// FIX - transpose after combining...
			
			// Read and Transpose

			Resampler resampler;
			HISSTools_RefPtr<float> baseIR(file.getFrames());
		
			file.readChannel(baseIR.get(), file.getFrames(), 0);
			float *IRTempL = resampler.process(baseIR.get(), file.getFrames(), outLength, file.getSamplingRate(), outSR);
			file.seek();
			file.readChannel(baseIR.get(), file.getFrames(), 1 % file.getChannels());
			float *IRTempR = resampler.process(baseIR.get(), file.getFrames(), outLength, file.getSamplingRate(), outSR);
		
			filesL[i] = HISSTools_RefPtr<double>(outLength);
			filesR[i] = HISSTools_RefPtr<double>(outLength);
			
			for (unsigned long j = 0; j < outLength; j++)
				filesL[i][j] = IRTempL[j];
			
			for (unsigned long j = 0; j < outLength; j++)
				filesR[i][j] = IRTempR[j];
			
			delete[] IRTempL;
			delete[] IRTempR;
		}
	}
	
	// Get Final Size
	
	long delays[4];
	
	for (int i = 0; i < 4; i++)
		delays[i] = DelayInSamps(i);
	
	int finalLength = 0;
	
	for (int i = 0; i < 4; i++)
		if (filesL[i].getSize() && filesL[i].getSize() + delays[i] > finalLength)
			finalLength = filesL[i].getSize() + delays[i];
	
	// Combine

	// Left
	
	IRL = HISSTools_RefPtr<double>(finalLength);
	memset(IRL.get(), 0, finalLength * sizeof(double));
	
	for (int i = 0; i < 4; i++)
	{
		double mul = GetParam(kSource1 + i)->Value() * DBToAmp(GetParam(kIRVolume1 + diff)->Value());
		long delay = delays[i];
		
		for (unsigned long j = 0; j < filesL[i].getSize(); j++)
			IRL[j + delay] += filesL[i][j] * mul;
	}
	
	// Right
	
	IRR = HISSTools_RefPtr<double>(finalLength);
	memset(IRR.get(), 0, finalLength * sizeof(double));
	
	for (int i = 0; i < 4; i++)
	{
		double mul = GetParam(kSource1 + i)->Value() * DBToAmp(GetParam(kIRVolume1 + diff)->Value());
		long delay = delays[i];

		for (unsigned long j = 0; j < filesR[i].getSize(); j++)
			IRR[j + delay] += filesR[i][j] * mul;
	}
}

void OctetViolins::LoadIRs()
{
	bool changed = false;
	bool numIRsChange = UpdateParamCache(kNumIRs, kNumIRs + 1);
	bool micChange = UpdateParamCache(kSource1, kDelay4 + 1);
	bool soloChanged = GetSoloChanged();
	
	for (int i = 0; i < 3 ; i++)
	{
		int diff = i * (kIR2 - kIR1);

		if (soloChanged || numIRsChange || micChange || UpdateParamCache(kIR1 + diff, kIR1 + diff + (kIR2 - kIR1)))
			changed = true;
		else
		{
			SetChanged(i, false);
			continue;
		}
		
		SetChanged(i, true);
		
		if (i < GetParam(kNumIRs)->Int() && !GetParam(kIRMute1 + diff)->Bool() && (!mSolo || GetParam(kIRSolo1 + diff)->Bool()))
		{
			// Load Files
			
			LoadFiles(diff, mIRs[i][0], mIRs[i][1]);
			
			HISSTools_RefPtr<double> IRL = mIRs[i][0];
			HISSTools_RefPtr<double> IRR = mIRs[i][1];
			
			unsigned long outLength = mIRs[i][0].getSize();

			// Filter
			
			if (GetParam(kIRHPFOn1 + diff)->Bool())
			{
				ButterworthHPF<double> HPF = ButterworthHPF<double>(2);
				 
				HPF.set(GetParam(kIRHPFFreq1 + diff)->Value(), 2, mSamplingRate);
			 
				for (unsigned long j = 0; j < outLength; j++)
					 IRL[j] = HPF(IRL[j]);
				
				HPF.reset();
				
				for (unsigned long j = 0; j < outLength; j++)
					IRR[j] = HPF(IRR[j]);
			}
			
			if (GetParam(kIRLPFOn1 + diff)->Bool())
			{
				ButterworthLPF<double> LPF = ButterworthLPF<double>(2);
				
				LPF.set(GetParam(kIRLPFFreq1 + diff)->Value(), 2, mSamplingRate);
				
				for (unsigned long j = 0; j < outLength; j++)
					IRL[j] = LPF(IRL[j]);
				
				LPF.reset();
				
				for (unsigned long j = 0; j < outLength; j++)
					IRR[j] = LPF(IRR[j]);
			}
		}
		else
			mIRs[i][0] = mIRs[i][1] = HISSTools_RefPtr<double>();
	}

	if (changed)
	{
		SetChanged(3, true);
		MixIRs();
		UpdateIRsAndDisplay();
	}
	else
		SetChanged(3, false);
}

void OctetViolins::MixIRs()
{
	long maxLength = 0;
	
	for (int i = 0; i < 3 ; i++)
		if (maxLength < mIRs[i][0].getSize())
			maxLength = mIRs[i][0].getSize();
	
	for (int i = 0; i < 3 ; i++)
		if (maxLength < mIRs[i][1].getSize())
			maxLength = mIRs[i][1].getSize();
	
	mIRs[3][0] = HISSTools_RefPtr<double>(maxLength);
	mIRs[3][1] = HISSTools_RefPtr<double>(maxLength);
	
	// Left

	memset(mIRs[3][0].get(), 0, maxLength * sizeof(double));

	for (int i = 0; i < 3 ; i++)
		for (unsigned long j = 0; j < mIRs[i][0].getSize(); j++)
			mIRs[3][0][j] += mIRs[i][0][j];
	
	// Right
	
	memset(mIRs[3][1].get(), 0, maxLength * sizeof(double));

	for (int i = 0; i < 3 ; i++)
		for (unsigned long j = 0; j < mIRs[i][1].getSize(); j++)
			mIRs[3][1][j] += mIRs[i][1][j];
}



void OctetViolins::SetChanged(int i, bool state)
{
	IMutexLock lock(this);
	
	mChanged[i] = state;
}

void OctetViolins::UpdateIRsAndDisplay(bool displayOnly)
{
	IMutexLock lock(this);

	if (!displayOnly)
		mUpdateAudioEngine = true;
	
	for (int i = 0; i < 4; i ++)
		if (mChanged[i])
			DisplaySpectrum(mIRs[i], i, mSamplingRate);
}

void OctetViolins::FadeIR(HISSTools_RefPtr <double> ir,  uintptr_t fadeIn,  uintptr_t fadeOut)
{
	uintptr_t sizeM1 = ir.getSize() - 1;
	
	// Fade in
	
	for (int i = 0; i < fadeIn; i++)
		ir[i] *= i / ((double) fadeIn);
	
	// Fade out
	
	for (int i = 0; i < fadeOut; i++)
		ir[sizeM1 - i] *= i / ((double) fadeOut);
}


void OctetViolins::DisplaySpectrum(HISSTools_RefPtr <double> IR, unsigned long index, double samplingRate)
{
	if (IR.getSize() == 0)
	{
		PowerSpectrum nullSpectrum(0);
		
		mSpectralDisplay->inputSpectrum(&nullSpectrum, index);
		mSpectralDisplay->setCurveDisplay(false, index);
	}
	else
	{
		// Transform and send to display
		
		Spectrum fSpectrum(IR.getSize());
		PowerSpectrum pSpectrum(fSpectrum.getFFTSize());
		HISSTools_FFT fft(IR.getSize());
		
		fft.timeToSpectrum(IR.get(), &fSpectrum, IR.getSize(), 0, samplingRate);
		pSpectrum.calcPowerSpectrum(&fSpectrum);
		
		mSpectralDisplay->inputSpectrum(&pSpectrum, index);
		mSpectralDisplay->setCurveDisplay(true, index);
	}
}

bool OctetViolins::UpdateParamCache(int start, int end)
{
	IMutexLock lock(this);

	bool change = false;
	
	for (int i = start; i < end; i++)
	{
		double param = GetParam(i)->GetNormalized();
		
		if (i == kIRVisible ||
			(i == kIRHPFFreq1 && !GetParam(kIRHPFOn1)->Bool()) ||
			(i == kIRHPFFreq2 && !GetParam(kIRHPFOn2)->Bool()) ||
			(i == kIRHPFFreq3 && !GetParam(kIRHPFOn3)->Bool()) ||
			(i == kIRLPFFreq1 && !GetParam(kIRLPFOn1)->Bool()) ||
			(i == kIRLPFFreq2 && !GetParam(kIRLPFOn2)->Bool()) ||
			(i == kIRLPFFreq3 && !GetParam(kIRLPFOn3)->Bool())
			)
			param = -1.0;
		
		if (mParamCache[i] != param)
			change = true;
		
		mParamCache[i] = param;
	}
			
	return change;
}

void OctetViolins::ClearParamCache()
{
	IMutexLock lock(this);

	for (int i = 0; i < kNumParams; i++)
		mParamCache[i] = -1.0;
}


void OctetViolins::CheckVisibleIR()
{
	int num = GetParam(kNumIRs)->Int();
	int sel = GetParam(kIRVisible)->Int();
	
	if (sel > num)
		SetIRDisplay(num - 1, true);
}

void OctetViolins::SetIRDisplay(int i, bool setParam)
{
	UpdateControlAndParam(i == 0, mSelectSwitches[0]);
	UpdateControlAndParam(i == 1, mSelectSwitches[1]);
	UpdateControlAndParam(i == 2, mSelectSwitches[2]);
	mIRTab->setTabFromPlug(i);
	
	if (setParam)
		GetParam(kIRVisible)->Set(i + 1);
	
	CheckVisibleIR();

}

void OctetViolins::OnParamChange(int paramIdx, ParamChangeSource source)
{
	IMutexLock lock(this);
	
	WDL_String path;
	
	bool solo = mSolo;
	mSolo = GetParam(kIRSolo1)->Bool() || GetParam(kIRSolo2)->Bool() || GetParam(kIRSolo3)->Bool();
	mSoloChanged = mSolo != solo;
	
	if (paramIdx == kNumIRs)
	{
		int num = GetParam(kNumIRs)->Int();
		
		CheckVisibleIR();
		
		mSelectSwitches[1]->Hide(num < 2);
		mSelectSwitches[2]->Hide(num < 3);
	}
	
	switch (paramIdx)
	{
		case kIRVisible:
			SetIRDisplay(GetParam(kIRVisible)->Int() - 1, false);
			break;
			
		case kIRSelect1:
			SetIRDisplay(0, true);
			break;
			
		case kIRSelect2:
			SetIRDisplay(1, true);
			break;
			
		case kIRSelect3:
			SetIRDisplay(2, true);
			break;
			
		default:
		
			mParamUpdated = true;
			if (!mThread)
				mThread = CreateThread(NULL, 0, LoadingThread, this, 0, NULL);
	 }
}


void OctetViolins::UpdateControlAndParam(double value, IControl *control, bool paramChange)
{
	control->SetValueFromPlug(control->GetParam()->GetNormalized(value));
	control->GetParam()->Set(value);
	
	if (paramChange == true)
		OnParamChange(control->ParamIdx(), kGUI);
}
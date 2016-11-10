
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

const char *paths[4][8] =
{
	{
		"IRs/Accelerometer_01_Treble.wav",
		"IRs/Accelerometer_02_Soprano.wav",
		"IRs/Accelerometer_03_Mezzo.wav",
		"IRs/Accelerometer_04_Alto.wav",
		"IRs/Accelerometer_05_Tenor.wav",
		"IRs/Accelerometer_06_Baritone.wav",
		"IRs/Accelerometer_07_Bass.wav",
		"IRs/Accelerometer_08_Contrabass.wav",
	},
	{
		"IRs/Neumann_01_Treble.wav",
		"IRs/Neumann_02_Soprano.wav",
		"IRs/Neumann_03_Mezzo.wav",
		"IRs/Neumann_04_Alto.wav",
		"IRs/Neumann_05_Tenor.wav",
		"IRs/Neumann_06_Baritone.wav",
		"IRs/Neumann_07_Bass.wav",
		"IRs/Neumann_08_Contrabass.wav",
	},
	{
		"IRs/Neumann_Pair_01_Treble.wav",
		"IRs/Neumann_Pair_02_Soprano.wav",
		"IRs/Neumann_Pair_03_Mezzo.wav",
		"IRs/Neumann_Pair_04_Alto.wav",
		"IRs/Neumann_Pair_05_Tenor.wav",
		"IRs/Neumann_Pair_06_Baritone.wav",
		"IRs/Neumann_Pair_07_Bass.wav",
		"IRs/Neumann_Pair_08_Contrabass.wav",
	},
	{
		"IRs/Gras_Pair_01_Treble.wav",
		"IRs/Gras_Pair_02_Soprano.wav",
		"IRs/Gras_Pair_03_Mezzo.wav",
		"IRs/Gras_Pair_04_Alto.wav",
		"IRs/Gras_Pair_05_Tenor.wav",
		"IRs/Gras_Pair_06_Baritone.wav",
		"IRs/Gras_Pair_07_Bass.wav",
		"IRs/Gras_Pair_08_Contrabass.wav",
	}
	
	
};

OctetViolins::OctetViolins(IPlugInstanceInfo instanceInfo)
:	IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo), mSamplingRate(44100.), mThread(NULL), mUpdateAudioEngine(false), mParamUpdated(false), mPresetIdx(-1)
{
	TRACE;

	// Define Parameters
	
	GetParam(kNumIRs)->InitInt("Number of IRs", 1, 1, 3, "");
	
	GetParam(kIRVisible)->InitInt("Selected IR", 1, 1, 3, "");
	GetParam(kIRVisible)->SetCanAutomate(false);
	
	for (int i = 0; i < 3; i++)
	{
		// FIX - better naming
		
		int offset = i * numIRParams();
		
		GetParam(kIR1 + offset)->InitEnum("Instrument", 2, 8);
		GetParam(kIR1 + offset)->SetDisplayText(0, "Treble");
		GetParam(kIR1 + offset)->SetDisplayText(1, "Soprano");
		GetParam(kIR1 + offset)->SetDisplayText(2, "Mezzo");
		GetParam(kIR1 + offset)->SetDisplayText(3, "Alto");
		GetParam(kIR1 + offset)->SetDisplayText(4, "Tenor");
		GetParam(kIR1 + offset)->SetDisplayText(5, "Baritone");
		GetParam(kIR1 + offset)->SetDisplayText(6, "Bass");
		GetParam(kIR1 + offset)->SetDisplayText(7, "Contrabass");
		
		GetParam(kIRVolume1 + offset)->InitDouble("Amp", 0.0, -30, 30, 0.1, "dB");
		GetParam(kIRTransposition1 + offset)->InitDouble("Transposition", 0, -12, 12, 0.1, "st");
		GetParam(kIRHPFFreq1 + offset)->InitDouble("HPF Freq", 1000, 10, 20000, 1.0, "Hz", "", 2.0);
		GetParam(kIRLPFFreq1 + offset)->InitDouble("LPF Freq", 1000, 10, 20000, 1.0, "Hz", "", 2.0);
		
		GetParam(kIRHPFSlope1 + offset)->InitEnum("HPF Slope", 0, 4);
		GetParam(kIRHPFSlope1 + offset)->SetDisplayText(0, "6dB/oct");
		GetParam(kIRHPFSlope1 + offset)->SetDisplayText(1, "12dB/oct");
		GetParam(kIRHPFSlope1 + offset)->SetDisplayText(2, "18dB/oct");
		GetParam(kIRHPFSlope1 + offset)->SetDisplayText(3, "24dB/oct");
		
		GetParam(kIRLPFSlope1 + offset)->InitEnum("LPF Slope", 0, 4);
		GetParam(kIRLPFSlope1 + offset)->SetDisplayText(0, "6dB/oct");
		GetParam(kIRLPFSlope1 + offset)->SetDisplayText(1, "12dB/oct");
		GetParam(kIRLPFSlope1 + offset)->SetDisplayText(2, "18dB/oct");
		GetParam(kIRLPFSlope1 + offset)->SetDisplayText(3, "24dB/oct");
		
		GetParam(kIRHPFOn1 + offset)->InitEnum("HPF On", 0, 2);
		GetParam(kIRHPFOn1 + offset)->SetDisplayText(0, "Off");
		GetParam(kIRHPFOn1 + offset)->SetDisplayText(1, "On");

		GetParam(kIRLPFOn1 + offset)->InitEnum("LPF On", 0, 2);
		GetParam(kIRLPFOn1 + offset)->SetDisplayText(0, "Off");
		GetParam(kIRLPFOn1 + offset)->SetDisplayText(1, "On");
		
		GetParam(kIRMute1 + offset)->InitEnum("Mute", 0, 2);
		GetParam(kIRMute1 + offset)->SetDisplayText(0, "Off");
		GetParam(kIRMute1 + offset)->SetDisplayText(1, "On");
		
		GetParam(kIRSolo1 + offset)->InitEnum("Solo", 0, 2);
		GetParam(kIRSolo1 + offset)->SetDisplayText(0, "Off");
		GetParam(kIRSolo1 + offset)->SetDisplayText(1, "On");
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

	//pGraphics->SetStrictDrawing(true);
	//pGraphics->SetAllowRetina(false);

	CreateControls(pGraphics);
	AttachGraphics(pGraphics);
	
	ClearParamCache();
	OnParamReset(kReset);
	
	for (int i = 0; i < numIRParams(); i++)
		mIRDefaultValues[i] = GetParam(kIR1 + i)->Value();
};

OctetViolins::~OctetViolins()
{
	// Wait for the loading thread to complete before we exit
	
	IMutexLock lock(this);
	
	HANDLE thread = mThread;
	
	lock.Destroy();
	
	if (thread)
		WaitForSingleObject(thread, INFINITE);
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
		HISSTools_Color_Spec *col5 = new HISSTools_Color_Spec(0.3, 0.3, 0.3, 0.9);
		HISSTools_Color_Spec *col6 = new HISSTools_Color_Spec(1.0, 0.1, 0.1, 0.9);
		
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
		
		addColorSpec("ButtonHandleOff", "remove", col5);
		addColorSpec("ButtonHandleOn", "remove", col6);
		
		addFlag("ValueDrawTriangle", "small", false);
		addFlag("ValueDrawLabel", "nolabel", false);
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
	
	//	Add IRs
	
	mAddIRButtons[0] = new AddIRButton(this, &mVecDraw, freqDispX + 80, dispHeight + 50, 70, 25);
	mAddIRButtons[1] = new AddIRButton(this, &mVecDraw, freqDispX + 160, dispHeight + 50, 70, 25);
	
	pGraphics->AttachControl(mAddIRButtons[0]);
	pGraphics->AttachControl(mAddIRButtons[1]);
	
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
	
	// Presets
	
	for (int i = 0; i < 5; i++)
		mPresetButtons[i + 0] = new PresetButton(this, &mVecDraw, i + 0, freqDispX + 540 + (i * 30), freqDispY + dispHeight + 20, 20, 20, "tight");
	
	for (int i = 0; i < 5; i++)
		mPresetButtons[i + 5] = new PresetButton(this, &mVecDraw, i + 5, freqDispX + 540 + (i * 30), freqDispY + dispHeight + 50, 20, 20, "tight");
	
	for (int i = 0; i < 10; i++)
	{
		pGraphics->AttachControl(mPresetButtons[i]);
		mPresetButtons[i]->SetValueFromPlug(i == mPresetIdx);
		mPresetButtons[i]->GrayOut(!mGUIPresets[i].Size());
	}

	// IR Parameters
	
	// Invisible Tabs
	
	mIRTab = new HISSTools_Invisible_Tabs(this, kIRVisible);

	for (int i = 0; i < 3; i++)
	{
		int offset = i * numIRParams();
		
		mSelections[i] = new HISSTools_Value(this, kIR1 + offset, &mVecDraw, freqDispX + 10, freqDispY + dispHeight + 60, 150, 30);
	
		char dialStyle1[64];
		char dialStyle2[64];
		char dialStyle3[64];
		
		sprintf(dialStyle1, "bipolar %d", i + 1);
		sprintf(dialStyle2, "bipolar %d", i + 1);
		sprintf(dialStyle3, "small %d", i + 1);
	
		mAmps[i] = new HISSTools_Dial(this, kIRVolume1 + offset, &mVecDraw, freqDispX + 10, freqDispY + dispHeight + 130, dialStyle1, &theDesign);
		mTranspositions[i] = new HISSTools_Dial(this, kIRTransposition1 + offset, &mVecDraw, freqDispX + 120, freqDispY + dispHeight + 130, dialStyle2, &theDesign);
		mHPFFreqs[i] = new HISSTools_Dial(this, kIRHPFFreq1 + offset, &mVecDraw, freqDispX + 245, freqDispY + dispHeight + 130, dialStyle3, &theDesign);
		mLPFFreqs[i] = new HISSTools_Dial(this, kIRLPFFreq1 + offset, &mVecDraw, freqDispX + 355, freqDispY + dispHeight + 130, dialStyle3, &theDesign);
		
		mHPFSlopes[i] = new HISSTools_Value(this, kIRHPFSlope1 + offset, &mVecDraw, freqDispX + 240, freqDispY + dispHeight + 230, 70, 20, "small nolabel", &theDesign);
		mLPFSlopes[i] = new HISSTools_Value(this, kIRLPFSlope1 + offset, &mVecDraw, freqDispX + 350, freqDispY + dispHeight + 230, 70, 20, "small nolabel", &theDesign);
		
		mHPFSwitches[i] = new HISSTools_Button(this, kIRHPFOn1 + offset, &mVecDraw, freqDispX + 240, freqDispY + dispHeight + 260, 70, 20);
		mLPFSwitches[i] = new HISSTools_Button(this, kIRLPFOn1 + offset, &mVecDraw, freqDispX + 350, freqDispY + dispHeight + 260, 70, 20);
	
		mMuteSwitches[i] = new HISSTools_Button(this, kIRMute1 + offset, &mVecDraw, freqDispX + 460, freqDispY + dispHeight + 230, 70, 20);
		mSoloSwitches[i] = new HISSTools_Button(this, kIRSolo1 + offset, &mVecDraw, freqDispX + 460, freqDispY + dispHeight + 190, 70, 20);
		
		pGraphics->AttachControl(mSelections[i]);
		pGraphics->AttachControl(mAmps[i]);
		pGraphics->AttachControl(mTranspositions[i]);
		pGraphics->AttachControl(mHPFFreqs[i]);
		pGraphics->AttachControl(mLPFFreqs[i]);
		pGraphics->AttachControl(mHPFSlopes[i]);
		pGraphics->AttachControl(mLPFSlopes[i]);
		pGraphics->AttachControl(mHPFSwitches[i]);
		pGraphics->AttachControl(mLPFSwitches[i]);
		pGraphics->AttachControl(mMuteSwitches[i]);
		pGraphics->AttachControl(mSoloSwitches[i]);
		
		mIRTab->attachControl(mSelections[i], i);
		mIRTab->attachControl(mAmps[i], i);
		mIRTab->attachControl(mTranspositions[i], i);
		mIRTab->attachControl(mHPFFreqs[i], i);
		mIRTab->attachControl(mLPFFreqs[i], i);
		mIRTab->attachControl(mHPFSlopes[i], i);
		mIRTab->attachControl(mLPFSlopes[i], i);
		mIRTab->attachControl(mHPFSwitches[i], i);
		mIRTab->attachControl(mLPFSwitches[i], i);
		mIRTab->attachControl(mMuteSwitches[i], i);
		mIRTab->attachControl(mSoloSwitches[i], i);
	}
	
	mRemoveIRButton = new RemoveIRButton(this, &mVecDraw, freqDispX + 460, freqDispY + dispHeight + 150, 70, 20, &theDesign);
	pGraphics->AttachControl(mRemoveIRButton);
							 
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

void OctetViolins::AddIR()
{
	IMutexLock lock(this);
	
	int numIRs = GetParam(kNumIRs)->Int();
	
	if (numIRs < 3)
	{
		GetParam(kNumIRs)->Set(numIRs + 1);
		OnParamChange(kNumIRs, kGUI);
		SetIRDisplay(numIRs, true);
	}
}

void OctetViolins::RemoveIR()
{
	IMutexLock lock(this);
	
	int numIRs = GetParam(kNumIRs)->Int();
	
	if (numIRs > 1)
	{
		int idx = GetParam(kIRVisible)->Int();
		
		int numParams = numIRParams();
		int offset = (idx - 1) * numParams;
		int copyOffset = (numIRs - 1) * numParams;
		
		for (int i = kIR1 + offset; i < (kIR1 + numParams * 2); i++)
			GetParam(i)->Set(GetParam(i + numParams)->Value());
		
		for (int i = 0; i < numParams; i++)
			GetParam(kIR1 + i + copyOffset)->Set(mIRDefaultValues[i]);

		RedrawParamControls();
		GetParam(kNumIRs)->Set(numIRs - 1);
		OnParamChange(kNumIRs, kGUI);
	}
}

void OctetViolins::SavePreset(int idx)
{
	SerializeParams(&mGUIPresets[idx]);
	mPresetButtons[idx]->GrayOut(!mGUIPresets[idx].Size());
	SetPreset(idx);
}

void OctetViolins::SetPreset(int idx)
{
	if (idx < 0)
		return;
	
	for (int i = 0; i < 10; i++)
		mPresetButtons[i]->SetValueFromPlug(i == idx);
	
	if (mGUIPresets[idx].Size())
		UnserializeParams(&mGUIPresets[idx], 0);
	
	mPresetIdx = idx;
}

void OctetViolins::RemovePreset(int idx)
{
	mGUIPresets[idx].Resize(0);
	mPresetButtons[idx]->SetValueFromPlug(0.0);
	mPresetButtons[idx]->GrayOut(true);
	
	if (mPresetIdx == idx)
		mPresetIdx = -1;
}

bool OctetViolins::SerializeState(ByteChunk* pChunk)
{
	IMutexLock lock(this);

	bool savedOK = true;

	pChunk->Put(&mPresetIdx);
	
	for (int i = 0; i < 10; i++)
	{
		int size = mGUIPresets[i].Size();
		
		pChunk->Put(&size);
		
		if (size)
			savedOK &= (pChunk->PutChunk(&mGUIPresets[i]) > 0);
	}
	
	return savedOK && SerializeParams(pChunk);
}

int OctetViolins::UnserializeState(ByteChunk* pChunk, int startPos)
{
	IMutexLock lock(this);

	startPos = pChunk->Get(&mPresetIdx, startPos);
	
	for (int i = 0; i < 10; i++)
	{
		int size;
		
		startPos = pChunk->Get(&size, startPos);
		mGUIPresets[i].Resize(size);
		
		if (size)
			startPos = pChunk->GetBytes(mGUIPresets[i].GetBytes(), size, startPos);
	}
	
	SetPreset(mPresetIdx);
	
	return UnserializeParams(pChunk, startPos);
}

void OctetViolins::WriteState(const char *filePath)
{
	ByteChunk serialisedFile;

	if (!SerializeState(&serialisedFile))
		return;
	
	std::streampos size;
	std::ofstream file(filePath, std::ios::out|std::ios::binary);
	
	if (file.is_open())
	{
		file.write((char *)serialisedFile.GetBytes(), serialisedFile.Size());
		file.close();
		
		if (!file.fail())
			return;
	}
}

void OctetViolins::ReadState(const char *filePath)
{
	ByteChunk serialisedFile;
	
	std::streampos size;
	
	std::ifstream file(filePath, std::ios::in|std::ios::binary|std::ios::ate);
	
	if (file.is_open())
	{
		size = file.tellg();
		
		serialisedFile.Resize((int) size);
		
		file.seekg (0, std::ios::beg);
		file.read((char *)serialisedFile.GetBytes(), size);
		file.close();
		
		if (!file.fail())
			return;
	}
	
	UnserializeState(&serialisedFile, 0);
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

void OctetViolins::LoadFiles(int offset, HISSTools_RefPtr<double> &IRL, HISSTools_RefPtr<double> &IRR)
{
	unsigned long outLength;
	
	double outSR = mSamplingRate * pow(2.0, -GetParam(kIRTransposition1 + offset)->Value() / 12.0);
	
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
		
		snprintf(fullPath, 4096, "%s/Contents/Resources/%s", bundlePath, paths[i][GetParam(kIR1 + offset)->Int()]);

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
		double mul = GetParam(kSource1 + i)->Value() * DBToAmp(GetParam(kIRVolume1 + offset)->Value());
		long delay = delays[i];
		
		for (unsigned long j = 0; j < filesL[i].getSize(); j++)
			IRL[j + delay] += filesL[i][j] * mul;
	}
	
	// Right
	
	IRR = HISSTools_RefPtr<double>(finalLength);
	memset(IRR.get(), 0, finalLength * sizeof(double));
	
	for (int i = 0; i < 4; i++)
	{
		double mul = GetParam(kSource1 + i)->Value() * DBToAmp(GetParam(kIRVolume1 + offset)->Value());
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
	int numIRs = GetParam(kNumIRs)->Int();
	
	for (int i = 0; i < 3 ; i++)
	{
		int offset = i * numIRParams();

		if (soloChanged || numIRsChange || micChange || UpdateParamCache(kIR1 + offset, kIR1 + offset + numIRParams()))
			changed = true;
		else
		{
			SetChanged(i, false);
			continue;
		}
		
		SetChanged(i, true);
		
		if (i < GetParam(kNumIRs)->Int() && !GetParam(kIRMute1 + offset)->Bool() && (!mSolo || GetParam(kIRSolo1 + offset)->Bool()))
		{
			// Load Files
			
			LoadFiles(offset, mIRs[i][0], mIRs[i][1]);
			
			HISSTools_RefPtr<double> IRL = mIRs[i][0];
			HISSTools_RefPtr<double> IRR = mIRs[i][1];
			
			unsigned long outLength = mIRs[i][0].getSize();

			// Filter
			
			if (GetParam(kIRHPFOn1 + offset)->Bool())
			{
				ButterworthHPF<double> HPF = ButterworthHPF<double>(4);
				
				HPF.set(GetParam(kIRHPFFreq1 + offset)->Value(), GetParam(kIRHPFSlope1 + offset)->Int() + 1, mSamplingRate);
			 
				for (unsigned long j = 0; j < outLength; j++)
					 IRL[j] = HPF(IRL[j]);
				
				HPF.reset();
				
				for (unsigned long j = 0; j < outLength; j++)
					IRR[j] = HPF(IRR[j]);
			}
			
			if (GetParam(kIRLPFOn1 + offset)->Bool())
			{
				ButterworthLPF<double> LPF = ButterworthLPF<double>(4);
				
				LPF.set(GetParam(kIRLPFFreq1 + offset)->Value(), GetParam(kIRLPFSlope1 + offset)->Int() + 1, mSamplingRate);
				
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
		MixIRs(numIRs);
		UpdateIRsAndDisplay();
	}
	else
		SetChanged(3, false);
}

void OctetViolins::MixIRs(int numIRs)
{
	bool normalise = true;
	double gain = normalise ? 1.0 / numIRs : 1.0;
	
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
			mIRs[3][0][j] += (mIRs[i][0][j] * gain);
	
	// Right
	
	memset(mIRs[3][1].get(), 0, maxLength * sizeof(double));

	for (int i = 0; i < 3 ; i++)
		for (unsigned long j = 0; j < mIRs[i][1].getSize(); j++)
			mIRs[3][1][j] += (mIRs[i][1][j] * gain);
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
			(i == kIRLPFFreq3 && !GetParam(kIRLPFOn3)->Bool()) ||
			(i == kIRHPFSlope1 && !GetParam(kIRHPFOn1)->Bool()) ||
			(i == kIRHPFSlope2 && !GetParam(kIRHPFOn2)->Bool()) ||
			(i == kIRHPFSlope3 && !GetParam(kIRHPFOn3)->Bool()) ||
			(i == kIRLPFSlope1 && !GetParam(kIRLPFOn1)->Bool()) ||
			(i == kIRLPFSlope2 && !GetParam(kIRLPFOn2)->Bool()) ||
			(i == kIRLPFSlope3 && !GetParam(kIRLPFOn3)->Bool())
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
		mAddIRButtons[0]->Hide(num != 1);
		mAddIRButtons[1]->Hide(num != 2);
		mRemoveIRButton->GrayOut(num < 2);
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
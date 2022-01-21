
#include "IControl.h"

#include "OctetViolins.h"
#include "OctetViolins_Controls.h"
#include "IPlug_include_in_plug_src.h"

#include "config.h"
#include <algorithm>
#include <cmath>

#include "Resampler.h"
#include "Biquad.h"

#include "HISSTools_Library/AudioFile/IAudioFile.h"
#include "HISSTools_Library/SpectralProcessor.hpp"

// Number of Programs

const int kNumPrograms = 1;

// IR Paths

const char *correctionPath = "IRs/Correction.wav";

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

void GetIRPath(WDL_String& string, const char *filePath)
{
	// FIX - cross platform
	
	char bundlePath[4096];
	
	CFBundleRef requestedBundle = CFBundleGetBundleWithIdentifier(CFSTR(BUNDLE_ID));
	CFURLRef url = CFBundleCopyBundleURL(requestedBundle);
	CFURLGetFileSystemRepresentation(url, true, (UInt8 *) bundlePath, (CFIndex) 4096);
	
	string.Set(bundlePath);
	string.Append("/Contents/Resources/");
	string.Append(filePath);
}

// Visual Design

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
		HISSTools_Color_Spec *col7 = new HISSTools_Color_Spec(0.4, 0.4, 0.9, 0.9);
		
		addColorSpec("SpectralCurve1", col1);
		addColorSpec("SpectralCurve2", col2);
		addColorSpec("SpectralCurve3", col3);
		addColorSpec("SpectralCurve4", col4);
		
		addColorSpec("ButtonHandleOn", "1", col1);
		addColorSpec("ButtonHandleOn", "2", col2);
		addColorSpec("ButtonHandleOn", "3", col3);
		addColorSpec("ButtonHandleOn", "preset", col7);
		
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

// Loading Thread Entry Point

DWORD LoadingThread(LPVOID plugParam)
{
	OctetViolins *plug = (OctetViolins *) plugParam;
	
	plug->LoadUntilUpdated();
	
	return NULL;
}

// Constructor and Destructor

OctetViolins::OctetViolins(InstanceInfo info)
: Plugin(info, MakeConfig(kNumParams, kNumPrograms)), mSamplingRate(44100.), mThread(NULL), mUpdateAudioEngine(false), mParamUpdated(false), mPresetIdx(-1)
{
	TRACE;

	// Define Parameters
	
	GetParam(kNumIRs)->InitInt("Number of IRs", 1, 1, 3, "");
	
	GetParam(kIRVisible)->InitInt("Selected IR", 1, 1, 3, "", IParam::kFlagCannotAutomate);
	
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
		GetParam(kIRHPFFreq1 + offset)->InitDouble("HPF Freq", 1000, 10, 20000, 1.0, "Hz", IParam::kFlagsNone, "", IParam::ShapePowCurve(2.0));
		GetParam(kIRLPFFreq1 + offset)->InitDouble("LPF Freq", 1000, 10, 20000, 1.0, "Hz", IParam::kFlagsNone, "", IParam::ShapePowCurve(2.0));
		
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
	
	GetParam(kSource1)->InitDouble("Accel", 0.0, 0.0, 1.0, 0.01, "", IParam::kFlagsNone, "", IParam::ShapePowCurve(2.0));
	GetParam(kSource2)->InitDouble("Cardioid", 1.0, 0.0, 1.0, 0.01, "", IParam::kFlagsNone, "", IParam::ShapePowCurve(2.0));
	GetParam(kSource3)->InitDouble("Pair 1", 0.0, 0.0, 1.0, 0.01, "", IParam::kFlagsNone, "", IParam::ShapePowCurve(2.0));
	GetParam(kSource4)->InitDouble("Pair 2", 0.0, 0.0, 1.0, 0.01, "", IParam::kFlagsNone, "", IParam::ShapePowCurve(2.0));
	
	GetParam(kDelay1)->InitDouble("Delay", 0.0, 0.0, 20.0, 0.1, "ms");
	GetParam(kDelay2)->InitDouble("Delay", 0.0, 0.0, 20.0, 0.1, "ms");
	GetParam(kDelay3)->InitDouble("Delay", 0.0, 0.0, 20.0, 0.1, "ms");
	GetParam(kDelay4)->InitDouble("Delay", 0.0, 0.0, 20.0, 0.1, "ms");
	
	GetParam(kIRSelect1)->InitBool("IR 1", true, "", IParam::kFlagCannotAutomate);
    GetParam(kIRSelect2)->InitBool("IR 2", false, "", IParam::kFlagCannotAutomate);
    GetParam(kIRSelect3)->InitBool("IR 3", false, "", IParam::kFlagCannotAutomate);
	
	GetParam(kCorrection)->InitBool("Correction", false);

	MakeDefaultPreset("-", kNumPrograms);
	
	ClearParamCache();
	OnParamReset(kReset);
	
	for (int i = 0; i < numIRParams(); i++)
		mIRDefaultValues[i] = GetParam(kIR1 + i)->Value();
};

OctetViolins::~OctetViolins()
{
	// Wait for the loading thread to complete before we exit
	
    HANDLE thread = nullptr;
    
    if (true) // Scope for lock
    {
        WDL_MutexLock lock(&mMutex);
        thread = mThread;
        mThread = NULL;
    }
		
	if (thread)
		WaitForSingleObject(thread, INFINITE);
}

// GUI Creation

IGraphics* OctetViolins::CreateGraphics()
{
  return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, 60, 1.);
}

void OctetViolins::LayoutUI(IGraphics *pGraphics)
{
    if (pGraphics->NControls())
        return;
        
    pGraphics->LoadFont("Arial Bold", "Arial", ETextStyle::Bold);

	IColor bgrb = IColor(255, 140, 140, 140);
				  
	int freqDispX = 30;
	int freqDispY = 30;
	int dispWidth = 700;
	int dispHeight = 300;

	// Main Displays

	mSpectralDisplay = new HISSTools_Spectral_Display(freqDispX, freqDispY, dispWidth, dispHeight, 1050000, 4, 0, &theDesign);
	
	mSpectralDisplay->SetRanges(20.0, 22050.0, -80.0, 20.0);
	
	pGraphics->AttachControl(mSpectralDisplay);
	
	// Num IRs and Selection
	
	auto select1 = new HISSTools_Button(kIRSelect1, freqDispX, dispHeight + 50, 70, 25, "1", &theDesign);
    auto select2 = new HISSTools_Button(kIRSelect2, freqDispX + 80, dispHeight + 50, 70, 25, "2", &theDesign);
    auto select3 = new HISSTools_Button(kIRSelect3, freqDispX + 160, dispHeight + 50, 70, 25, "3", &theDesign);
	
	pGraphics->AttachControl(select1, kSelect1);
    pGraphics->AttachControl(select2, kSelect2);
    pGraphics->AttachControl(select3, kSelect3);
	
	//	Add IRs
	
	auto add1 = new AddIRButton(this, freqDispX + 80, dispHeight + 50, 70, 25);
	auto add2 = new AddIRButton(this, freqDispX + 160, dispHeight + 50, 70, 25);
	
	pGraphics->AttachControl(add1, kAdd1);
	pGraphics->AttachControl(add2, kAdd2);
	
	// FIX - HACK
	
	dispHeight += 70;

	pGraphics->AttachControl(new HISSTools_Value(kSource1, freqDispX + 200, freqDispY + dispHeight + 20, 60, 20));
	pGraphics->AttachControl(new HISSTools_Value(kSource2, freqDispX + 280, freqDispY + dispHeight + 20, 60, 20));
	pGraphics->AttachControl(new HISSTools_Value(kSource3, freqDispX + 360, freqDispY + dispHeight + 20, 60, 20));
	pGraphics->AttachControl(new HISSTools_Value(kSource4, freqDispX + 440, freqDispY + dispHeight + 20, 60, 20));

	pGraphics->AttachControl(new HISSTools_Value(kDelay1, freqDispX + 200, freqDispY + dispHeight + 70, 60, 20));
	pGraphics->AttachControl(new HISSTools_Value(kDelay2, freqDispX + 280, freqDispY + dispHeight + 70, 60, 20));
	pGraphics->AttachControl(new HISSTools_Value(kDelay3, freqDispX + 360, freqDispY + dispHeight + 70, 60, 20));
	pGraphics->AttachControl(new HISSTools_Value(kDelay4, freqDispX + 440, freqDispY + dispHeight + 70, 60, 20));
	
	// Presets
	
	for (int i = 0; i < 5; i++)
		mPresetButtons[i + 0] = new PresetButton(this, i + 0, freqDispX + 540 + (i * 30), freqDispY + dispHeight + 20, 20, 20, "tight preset", &theDesign);
	
	for (int i = 0; i < 5; i++)
		mPresetButtons[i + 5] = new PresetButton(this, i + 5, freqDispX + 540 + (i * 30), freqDispY + dispHeight + 50, 20, 20, "tight preset", &theDesign);
	
	for (int i = 0; i < 10; i++)
	{
		pGraphics->AttachControl(mPresetButtons[i]);
		mPresetButtons[i]->SetValueFromDelegate(i == mPresetIdx);
		mPresetButtons[i]->SetDisabled(!mGUIPresets[i].Size());
	}
	
	pGraphics->AttachControl(new FileSaveLoad(this, "Save", freqDispX + 540, freqDispY + dispHeight + 90, 65, 20, EFileAction::Save, "tight"));
	pGraphics->AttachControl(new FileSaveLoad(this, "Load", freqDispX + 615, freqDispY + dispHeight + 90, 65, 20, EFileAction::Open, "tight"));
	
	// IR Parameters
	
	// Invisible Tabs
	
	auto tab = new HISSTools_Invisible_Tabs(kIRVisible);

	for (int i = 0; i < 3; i++)
	{
		int offset = i * numIRParams();
		
		auto selection = new HISSTools_Value(kIR1 + offset, freqDispX + 10, freqDispY + dispHeight + 60, 150, 30);
	
		char dialStyle1[64];
		char dialStyle2[64];
		char dialStyle3[64];
		
		sprintf(dialStyle1, "bipolar %d", i + 1);
		sprintf(dialStyle2, "bipolar %d", i + 1);
		sprintf(dialStyle3, "small %d", i + 1);
	
		auto amp = new HISSTools_Dial(kIRVolume1 + offset, freqDispX + 10, freqDispY + dispHeight + 130, dialStyle1, &theDesign);
		auto transposition = new HISSTools_Dial(kIRTransposition1 + offset, freqDispX + 120, freqDispY + dispHeight + 130, dialStyle2, &theDesign);
        auto hpfFreq = new HISSTools_Dial(kIRHPFFreq1 + offset, freqDispX + 245, freqDispY + dispHeight + 130, dialStyle3, &theDesign);
		auto lpfFreq = new HISSTools_Dial(kIRLPFFreq1 + offset, freqDispX + 355, freqDispY + dispHeight + 130, dialStyle3, &theDesign);
		
        auto hpfSlope = new HISSTools_Value(kIRHPFSlope1 + offset, freqDispX + 240, freqDispY + dispHeight + 230, 70, 20, "small nolabel", &theDesign);
        auto lpfSlope = new HISSTools_Value(kIRLPFSlope1 + offset, freqDispX + 350, freqDispY + dispHeight + 230, 70, 20, "small nolabel", &theDesign);
		
        auto hpfSwitch = new HISSTools_Button(kIRHPFOn1 + offset, freqDispX + 240, freqDispY + dispHeight + 260, 70, 20);
        auto lpfSwitch = new HISSTools_Button(kIRLPFOn1 + offset, freqDispX + 350, freqDispY + dispHeight + 260, 70, 20);
	
		auto muteSwitch = new HISSTools_Button(kIRMute1 + offset, freqDispX + 460, freqDispY + dispHeight + 230, 70, 20);
        auto soloSwitch = new HISSTools_Button(kIRSolo1 + offset, freqDispX + 460, freqDispY + dispHeight + 190, 70, 20);
		
		pGraphics->AttachControl(selection);
		pGraphics->AttachControl(amp);
		pGraphics->AttachControl(transposition);
		pGraphics->AttachControl(hpfFreq);
		pGraphics->AttachControl(lpfFreq);
		pGraphics->AttachControl(hpfSlope);
		pGraphics->AttachControl(lpfSlope);
		pGraphics->AttachControl(hpfSwitch);
		pGraphics->AttachControl(lpfSwitch);
		pGraphics->AttachControl(muteSwitch);
		pGraphics->AttachControl(soloSwitch);
		
        tab->attachControl(selection, i);
        tab->attachControl(amp, i);
        tab->attachControl(transposition, i);
        tab->attachControl(hpfFreq, i);
        tab->attachControl(lpfFreq, i);
        tab->attachControl(hpfSlope, i);
        tab->attachControl(lpfSlope, i);
        tab->attachControl(hpfSwitch, i);
        tab->attachControl(lpfSwitch, i);
        tab->attachControl(muteSwitch, i);
        tab->attachControl(soloSwitch, i);
	}
	
	auto remove = new RemoveIRButton(this, freqDispX + 460, freqDispY + dispHeight + 150, 70, 20, &theDesign);
	pGraphics->AttachControl(remove, kDelete);
	
	pGraphics->AttachControl(new HISSTools_Button(kCorrection, freqDispX + 560, freqDispY + dispHeight + 150, 100, 20, "tight"));

	pGraphics->AttachControl(tab);
	pGraphics->AttachPanelBackground(bgrb);

    // Finalise Graphics
    
    pGraphics->EnableMouseOver(true);
        
	// Display curves
	
	UpdateIRsAndDisplay(true);
}

// GUI Interactions

void OctetViolins::AddIR()
{
    WDL_MutexLock lock(&mMutex);
	
	int numIRs = GetParam(kNumIRs)->Int();
	
	if (numIRs < 3)
	{
		GetParam(kNumIRs)->Set(numIRs + 1);
		OnParamChange(kNumIRs, kUI, 0);
        OnParamChangeUI(kNumIRs, kUI);
		SetIRDisplay(numIRs, true);
	}
}

void OctetViolins::RemoveIR()
{
    WDL_MutexLock lock(&mMutex);
	
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

		GetParam(kNumIRs)->Set(numIRs - 1);
        OnParamChange(kNumIRs, kUI, 0);
        OnParamChangeUI(kNumIRs, kUI);
        SendCurrentParamValuesFromDelegate();
	}
}

void OctetViolins::SavePreset(int idx)
{
	SerializeParams(mGUIPresets[idx]);
	mPresetButtons[idx]->SetDisabled(!mGUIPresets[idx].Size());
	SetPreset(idx);
}

void OctetViolins::SetPreset(int idx)
{
	if (idx < 0)
		return;
	
	for (int i = 0; i < 10; i++)
	{
		mPresetButtons[i]->SetValueFromDelegate(i == idx);
		mPresetButtons[i]->SetDisabled(!mGUIPresets[i].Size());
	}
	
	if (mGUIPresets[idx].Size())
		UnserializeParams(mGUIPresets[idx], 0);
	
	mPresetIdx = idx;
}

void OctetViolins::RemovePreset(int idx)
{
	mGUIPresets[idx].Resize(0);
	mPresetButtons[idx]->SetValueFromDelegate(0.0);
	mPresetButtons[idx]->SetDisabled(true);
	
	if (mPresetIdx == idx)
		mPresetIdx = -1;
}

// State and Presets

bool OctetViolins::SerializeState(IByteChunk& chunk) const
{
    WDL_MutexLock lock(&mMutex);

	bool savedOK = true;

    chunk.Put(&mPresetIdx);
	
	for (int i = 0; i < 10; i++)
	{
		int size = mGUIPresets[i].Size();
		
        chunk.Put(&size);
		
		if (size)
			savedOK &= (chunk.PutChunk(&mGUIPresets[i]) > 0);
	}
	
	return savedOK && SerializeParams(chunk);
}

int OctetViolins::UnserializeState(const IByteChunk& chunk, int startPos)
{
    WDL_MutexLock lock(&mMutex);

	startPos = chunk.Get(&mPresetIdx, startPos);
	
	for (int i = 0; i < 10; i++)
	{
		int size;
		
		startPos = chunk.Get(&size, startPos);
		mGUIPresets[i].Resize(size);
		
		if (size)
			startPos = chunk.GetBytes(mGUIPresets[i].GetData(), size, startPos);
	}
	
	SetPreset(mPresetIdx);
	
	return UnserializeParams(chunk, startPos);
}

void OctetViolins::WriteState(const char *filePath)
{
	IByteChunk serialisedFile;

	if (!SerializeState(serialisedFile))
		return;
	
	std::streampos size;
	std::ofstream file(filePath, std::ios::out|std::ios::binary);
	
	if (file.is_open())
	{
		file.write((char *)serialisedFile.GetData(), serialisedFile.Size());
		file.close();
	}
}

void OctetViolins::ReadState(const char *filePath)
{
	IByteChunk serialisedFile;
	
	std::streampos size;
	
	std::ifstream file(filePath, std::ios::in|std::ios::binary|std::ios::ate);
	
	if (file.is_open())
	{
		size = file.tellg();
		
		serialisedFile.Resize((int) size);
		
		file.seekg(0, std::ios::beg);
		file.read((char *)serialisedFile.GetData(), size);
		file.close();
		
		if (file.fail())
			return;
	}
	
	UnserializeState(serialisedFile, 0);
}

// Process and Reset

void OctetViolins::ProcessBlock(double** inputs, double** outputs, int nFrames)
{
	if (mUpdateAudioEngine && mConvolver.set(mIRs[3][0].get(), mIRs[3][1].get(), mIRs[3][0].getSize()))
		mUpdateAudioEngine = false;
	
	mConvolver.process((const double **) inputs, outputs, nFrames);
}

void OctetViolins::OnReset()
{
	TRACE;
    WDL_MutexLock lock(&mMutex);
	
	ClearParamCache();
	mSamplingRate = GetSampleRate();

	mConvolver.reset(mSamplingRate);
}

// Loading

void OctetViolins::LoadUntilUpdated()
{	
	while (GetParamUpdated())
		LoadIRs();
}

bool OctetViolins::GetParamUpdated()
{
    WDL_MutexLock lock(&mMutex);
	
	bool paramUpdated = mParamUpdated;
	mParamUpdated = false;
	
	if (!paramUpdated)
	{
		HANDLE thread = mThread;
		mThread = NULL;
		if (thread)
			CloseHandle(thread);
	}
	
	return paramUpdated;
}

bool OctetViolins::GetSoloChanged()
{
    WDL_MutexLock lock(&mMutex);

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
		WDL_String filePath;
		
		GetIRPath(filePath, paths[i][GetParam(kIR1 + offset)->Int()]);
		
		HISSTools::IAudioFile file(filePath.Get());
		
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
	
	long finalLength = 0;
	
	for (int i = 0; i < 4; i++)
		if (filesL[i].getSize() && filesL[i].getSize() + delays[i] > finalLength)
			finalLength = static_cast<long>(filesL[i].getSize()) + delays[i];
	
	// Combine

	// Left
	
	IRL = HISSTools_RefPtr<double>(finalLength);
    std::fill_n(IRL.get(), finalLength, 0.0);
	
	for (int i = 0; i < 4; i++)
	{
		double mul = GetParam(kSource1 + i)->Value() * DBToAmp(GetParam(kIRVolume1 + offset)->Value());
		long delay = delays[i];
		
		for (unsigned long j = 0; j < filesL[i].getSize(); j++)
			IRL[j + delay] += filesL[i][j] * mul;
	}
	
	// Right
	
	IRR = HISSTools_RefPtr<double>(finalLength);
    std::fill_n(IRR.get(), finalLength, 0.0);
	
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
	bool correctionChanged = UpdateParamCache(kCorrection, kCorrection + 1);
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

	if (correctionChanged || changed)
	{
		SetChanged(3, true);
		MixIRs(numIRs, GetParam(kCorrection)->Bool());
		UpdateIRsAndDisplay();
	}
	else
		SetChanged(3, false);
}

void OctetViolins::MixIRs(int numIRs, bool correctionOn)
{
	bool normalise = true;
	double gain = normalise ? 1.0 / numIRs : 1.0;
	
	HISSTools_RefPtr <double> correction(1);
	
	long maxLength = 0;
	
	// If using correction load and resample the IR
	
	if (correctionOn)
	{
		WDL_String filePath;
		
		GetIRPath(filePath, correctionPath);
	
		HISSTools::IAudioFile file(filePath.Get());
		
		if (file.isOpen() && !file.getErrorFlags())
		{
			Resampler resampler;
			HISSTools_RefPtr<float> correctionRaw(file.getFrames());
			unsigned long outLength;

			file.readChannel(correctionRaw.get(), file.getFrames(), 0);
			float *resampledTemp = resampler.process(correctionRaw.get(), file.getFrames(), outLength, file.getSamplingRate(), mSamplingRate);

			correction = HISSTools_RefPtr <double>(outLength);

			for (unsigned long i = 0; i < outLength; i++)
				correction[i] = resampledTemp[i];

			delete[] resampledTemp;
		}
	}
	
	// Calculate the max length of the IRs
	
	for (int i = 0; i < 3 ; i++)
		if (maxLength < mIRs[i][0].getSize())
			maxLength = mIRs[i][0].getSize();
	
	for (int i = 0; i < 3 ; i++)
		if (maxLength < mIRs[i][1].getSize())
			maxLength = mIRs[i][1].getSize();
	
	long finalLength = maxLength + correction.getSize() - 1;
	
	mIRs[3][0] = HISSTools_RefPtr<double>(finalLength);
	mIRs[3][1] = HISSTools_RefPtr<double>(finalLength);
	
	// Left

    std::fill_n(mIRs[3][0].get(), finalLength, 0.0);

	for (int i = 0; i < 3 ; i++)
		for (unsigned long j = 0; j < mIRs[i][0].getSize(); j++)
			mIRs[3][0][j] += (mIRs[i][0][j] * gain);
	
	// Right
	
    std::fill_n(mIRs[3][1].get(), finalLength, 0.0);

	for (int i = 0; i < 3 ; i++)
		for (unsigned long j = 0; j < mIRs[i][1].getSize(); j++)
			mIRs[3][1][j] += (mIRs[i][1][j] * gain);
	
	// Correction
	
	if (correctionOn && (correction.getSize() > 1))
	{
        auto edgeMode = spectral_processor<double>::EdgeMode::Linear;
        auto length = static_cast<uintptr_t>(maxLength);
        spectral_processor<double>::in_ptr correctPtr { correction.get(), correction.getSize() };
        spectral_processor<double> processor(finalLength);
        		
		// Apply Correction
				
        processor.convolve(mIRs[3][0].get(), { mIRs[3][0].get(), length }, correctPtr, edgeMode);
        processor.convolve(mIRs[3][1].get(), { mIRs[3][1].get(), length }, correctPtr, edgeMode);
	}
}

void OctetViolins::SetChanged(int i, bool state)
{
    WDL_MutexLock lock(&mMutex);
	
	mChanged[i] = state;
}

void OctetViolins::UpdateIRsAndDisplay(bool displayOnly)
{
    WDL_MutexLock lock(&mMutex);

	if (!displayOnly)
		mUpdateAudioEngine = true;
	
	for (int i = 0; i < 4; i ++)
		if (mChanged[i] || displayOnly)
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
    if (!GetUI())
        return;
    
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
        fSpectrum.setSamplingRate(mSamplingRate);
        pSpectrum.setSamplingRate(mSamplingRate);

        spectral_processor<double> processor(fSpectrum.getFFTSize());
        auto FFTSizeLog2 = processor.calc_fft_size_log2(IR.getSize());
        processor.rfft(*fSpectrum.getSpectrum(), IR.get(), IR.getSize(), FFTSizeLog2);
		
		pSpectrum.calcPowerSpectrum(&fSpectrum);
		
		mSpectralDisplay->inputSpectrum(&pSpectrum, index);
		mSpectralDisplay->setCurveDisplay(true, index);
	}
}

bool OctetViolins::UpdateParamCache(int start, int end)
{
    WDL_MutexLock lock(&mMutex);

	bool change = false;
	
	for (int i = start; i < end; i++)
	{
		double param = GetParam(i)->GetNormalized();
		
		if (i == kIRVisible ||
            ((i == kIRHPFFreq1 || i == kIRHPFSlope1) && !GetParam(kIRHPFOn1)->Bool()) ||
			((i == kIRHPFFreq2 || i == kIRHPFSlope2) && !GetParam(kIRHPFOn2)->Bool()) ||
			((i == kIRHPFFreq3 || i == kIRHPFSlope3) && !GetParam(kIRHPFOn3)->Bool()) ||
			((i == kIRLPFFreq1 || i == kIRLPFSlope1) && !GetParam(kIRLPFOn1)->Bool()) ||
			((i == kIRLPFFreq2 || i == kIRLPFSlope2) && !GetParam(kIRLPFOn2)->Bool()) ||
			((i == kIRLPFFreq3 || i == kIRLPFSlope3) && !GetParam(kIRLPFOn3)->Bool()))
			param = -1.0;
		
		if (mParamCache[i] != param)
			change = true;
		
		mParamCache[i] = param;
	}
			
	return change;
}

void OctetViolins::ClearParamCache()
{
    WDL_MutexLock lock(&mMutex);

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
    if (!GetUI())
        return;
    
	UpdateControlAndParam(i == 0, GetUI()->GetControlWithTag(kSelect1));
    UpdateControlAndParam(i == 1, GetUI()->GetControlWithTag(kSelect2));
    UpdateControlAndParam(i == 2, GetUI()->GetControlWithTag(kSelect3));
	
	if (setParam)
    {
		GetParam(kIRVisible)->Set(i + 1);
        SendParameterValueFromDelegate(kIRVisible, i + 1, false);
    }
	
	CheckVisibleIR();
}

// Parameters

void OctetViolins::OnParamChange(int paramIdx, EParamSource source, int sampleOffset)
{
    WDL_MutexLock lock(&mMutex);
		
	bool solo = mSolo;
	mSolo = GetParam(kIRSolo1)->Bool() || GetParam(kIRSolo2)->Bool() || GetParam(kIRSolo3)->Bool();
	mSoloChanged = mSolo != solo;
	
	switch (paramIdx)
	{
        case kIRVisible:
		case kIRSelect1:
		case kIRSelect2:
		case kIRSelect3:
			break;
			
		default:
			mParamUpdated = true;
			if (!mThread)
				mThread = CreateThread(NULL, 0, LoadingThread, this, 0, NULL);
	 }
}

void OctetViolins::OnParamChangeUI(int paramIdx, EParamSource source)
{
    WDL_MutexLock lock(&mMutex);
 
    if (!GetUI())
        return;
    
    if (paramIdx == kNumIRs)
    {
        int numIRs = GetParam(kNumIRs)->Int();
        
        CheckVisibleIR();
        
        GetUI()->GetControlWithTag(kSelect2)->Hide(numIRs < 2);
        GetUI()->GetControlWithTag(kSelect3)->Hide(numIRs < 3);
        GetUI()->GetControlWithTag(kAdd1)->Hide(numIRs != 1);
        GetUI()->GetControlWithTag(kAdd2)->Hide(numIRs != 2);
        GetUI()->GetControlWithTag(kDelete)->SetDisabled(numIRs < 2);
    }
    
    switch (paramIdx)
    {
        case kIRVisible:
            SetIRDisplay(GetParam(kIRVisible)->Int() - 1, false);
            break;
            
        case kIRSelect1:    SetIRDisplay(0, true);      break;
        case kIRSelect2:    SetIRDisplay(1, true);      break;
        case kIRSelect3:    SetIRDisplay(2, true);      break;
            
        default:
            break;
     }
}


void OctetViolins::UpdateControlAndParam(double value, IControl *control, bool paramChange)
{
    control->SetValueFromDelegate(control->GetParam()->GetNormalized());
	GetParam(control->GetParamIdx())->Set(value);
	
	if (paramChange)
		OnParamChangeUI(control->GetParamIdx(), kUI);
}

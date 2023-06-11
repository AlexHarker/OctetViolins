#ifndef __IPLUGEFFECT__
#define __IPLUGEFFECT__

#include "IPlug_include_in_plug_hdr.h"

#include "HISSTools_Controls.hpp"
#include "HISSTools_Design_Scheme.hpp"
#include "HISSTools_ThreadSafety.hpp"
#include "HISSTools_Spectral_Display.hpp"

#include "CrossfadedConvolution.h"

#include <thread>

enum EParams
{
    kNumIRs,
    kIRVisible,
    
    kIR1,
    kIRVolume1,
    kIRTransposition1,
    kIRHPFFreq1,
    kIRLPFFreq1,
    kIRHPFSlope1,
    kIRLPFSlope1,
    kIRHPFOn1,
    kIRLPFOn1,
    kIRMute1,
    kIRSolo1,
    
    kIR2,
    kIRVolume2,
    kIRTransposition2,
    kIRHPFFreq2,
    kIRLPFFreq2,
    kIRHPFSlope2,
    kIRLPFSlope2,
    kIRHPFOn2,
    kIRLPFOn2,
    kIRMute2,
    kIRSolo2,
    
    kIR3,
    kIRVolume3,
    kIRTransposition3,
    kIRHPFFreq3,
    kIRLPFFreq3,
    kIRHPFSlope3,
    kIRLPFSlope3,
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
    
    kCorrection,
    
    kNumParams
};

enum ETags
{
    kAdd1,
    kAdd2,
    kDelete,
    kSelect1,
    kSelect2,
    kSelect3,
    kSpectralDisplay,
    kPresetButtons
};
    

static inline int numIRParams() { return kIR2 - kIR1; }

using namespace iplug;
using namespace igraphics;

class OctetViolins : public Plugin
{
    enum MeasurementMode {kHarmonics, kEQ, kTone, kCalibration};
    
public:
    
    // Constructor and Destructor
    
    OctetViolins(InstanceInfo info);
    ~OctetViolins();
            
    // GUI Interactions
    
    void AddIR();
    void RemoveIR();
    
    void SavePreset(int idx);
    void SetPreset(int idx);
    void RemovePreset(int idx);
    
    // State and Presets
    
    bool SerializeState(IByteChunk& chunk) const override;
    int UnserializeState(const IByteChunk& chunk, int startPos) override;
    
    void WriteState(const char *filePath);
    void ReadState(const char *filePath);
    
    // Process and Reset
    
    void ProcessBlock(double** inputs, double** outputs, int nFrames) override;
    void OnReset() override;
    
    // Loading
    
    void LoadUntilUpdated();
    
    // Parameters
    
    void OnParamChange(int paramIdx, EParamSource source, int sampleOffset) override;
    void OnParamChangeUI(int paramIdx, EParamSource source) override;
    
private:
    
    // GUI Creation
    IGraphics* CreateGraphics() override;
    void LayoutUI(IGraphics *pGraphics) override;

    void CheckVisibleIR();
    void SetIRDisplay(int i, bool setParam);
    long DelayInSamps(int i);
    bool GetSoloChanged();
    void LoadFiles(int diff, HISSTools_RefPtr<double> &IRL, HISSTools_RefPtr<double> &IRR);
    void LoadIRs();
    void MixIRs(int numIRs, bool correctionOn);
    void SetChanged(int i, bool state);
    void UpdateIRsAndDisplay(bool displayOnly = false);
    void DisplaySpectrum(HISSTools_RefPtr <double> IR, unsigned long index, double samplingRate);
    void FadeIR(HISSTools_RefPtr <double> ir,  uintptr_t fadeIn,  uintptr_t fadeOut);
    
    void ClearParamCache();
    bool UpdateParamCache(int start = 0, int end = kNumParams);
    
    void UpdateControlAndParam(double value, IControl *control, bool paramChange = false);
    
    // DSP
    
    CrossfadedConvolution mConvolver;
    
    // IR Storage
    
    HISSTools_RefPtr <double> mIRs[4][2];
    
    // Default IR Setup
    
    double mIRDefaultValues[kIR2 - kIR1];
    
    // Temporary / Working Parameters
    
    double mSamplingRate;
    
    double mParamCache[kNumParams];
    
    std::thread mThread;
    
    mutable WDL_Mutex mMutex;

    int mChanged[4];
    
    bool mSolo;
    bool mSoloChanged;
    bool mParamUpdated;
    bool mThreadJoining;
    bool mUpdateAudioEngine;
    
    // Presets
    
    int mPresetIdx;
    IByteChunk mGUIPresets[10];
};

#endif


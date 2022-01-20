

#include "OctetViolins_Controls.h"
#include "OctetViolins.h"


// Add IR Button

AddIRButton::AddIRButton(OctetViolins *pPlug, double x, double y, double w, double h, const char *type, HISSTools_Design_Scheme *designScheme) : HISSTools_Button(kNoParameter, x, y, w, h, type, designScheme, "+"), mPlug(pPlug)
{}

void AddIRButton::OnMouseDown(int x, int y, IMouseMod* pMod)
{
    SetValue(1.0);
    SetDirty(false);
}

void AddIRButton::OnMouseUp(int x, int y, IMouseMod* pMod)
{
    SetValue(0.0);
    SetDirty(false);
    mPlug->AddIR();
}

// Remove IR Button

RemoveIRButton::RemoveIRButton(OctetViolins *pPlug, double x, double y, double w, double h, HISSTools_Design_Scheme *designScheme) : HISSTools_Button(kNoParameter, x, y, w, h, "remove", designScheme, "Delete"), mPlug(pPlug)
{}

void RemoveIRButton::OnMouseDown(int x, int y, IMouseMod* pMod)
{
    SetValue(1.0);
    SetDirty(false);
}

void RemoveIRButton::OnMouseDrag(int x, int y, int dX, int dY, IMouseMod* pMod)
{
    SetValue(mTargetRECT.Contains(x, y));
    SetDirty(false);
}

void RemoveIRButton::OnMouseUp(int x, int y, IMouseMod* pMod)
{
    OnMouseDrag(x, y, 0, 0, pMod);
    
    if (GetValue())
        mPlug->RemoveIR();
    
    SetValue(0.0);
    SetDirty();
}

// Preset Button

PresetButton::PresetButton::PresetButton(OctetViolins *pPlug, int idx, double x, double y, double w, double h, const char *type, HISSTools_Design_Scheme *designScheme) : HISSTools_Button(kNoParameter, x, y, w, h, type, designScheme), mPlug(pPlug), mIdx(idx)
{
    mDisplayName.SetFormatted(32, "%d", idx + 1);
    SetMouseEventsWhenDisabled(true);
}

void PresetButton::OnMouseDown(int x, int y, IMouseMod* pMod)
{
    // N.B. The plug in deal with updating graphical state for all buttons
    
    if (pMod->S && !pMod->A && !pMod->C)
        mPlug->SavePreset(mIdx);
    else if (IsDisabled())
        return;
    else if (pMod->A && pMod->C)
        mPlug->RemovePreset(mIdx);
    else
        mPlug->SetPreset(mIdx);
}

// File Save and Load Buttons

FileSaveLoad::FileSaveLoad(OctetViolins *pPlug, const char *label, double x, double y, double w, double h, EFileAction action, const char *type, HISSTools_Design_Scheme *designScheme)
: HISSTools_Button(kNoParameter, x, y, w, h, type, designScheme, label), mFileAction(action), mPlug(pPlug)
{}

void FileSaveLoad::OnMouseDown(int x, int y, IMouseMod* pMod)
{
    WDL_String fileName;
    WDL_String dir;
    
    SetValueFromDelegate(1.0);
    
    mPlug->GetUI()->PromptForFile(fileName, dir, mFileAction, "octet");
    
    if (mFileAction == EFileAction::Open)
        mPlug->ReadState(fileName.Get());
    else
        mPlug->WriteState(fileName.Get());
    
    SetValueFromDelegate(0.0);
}

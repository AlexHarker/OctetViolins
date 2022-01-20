

#include "OctetViolins.h"
#include "OctetViolins_Controls.h"


// Add IR Button

AddIRButton::AddIRButton(OctetViolins *pPlug, HISSTools_LICE_Vec_Lib *vecDraw, double x, double y, double w, double h, const char *type, HISSTools_Design_Scheme *designScheme) : HISSTools_Button(mPlug, -1, vecDraw, x, y, w, h, type, designScheme), mPlug(pPlug)
{
    mName = "+";
}

void AddIRButton::OnMouseDown(int x, int y, IMouseMod* pMod)
{
    mValue = 1.0;
    SetDirty(false);
}

void AddIRButton::OnMouseUp(int x, int y, IMouseMod* pMod)
{
    mValue = 0.0;
    SetDirty(false);
    mPlug->AddIR();
}

// Remove IR Button

RemoveIRButton::RemoveIRButton(OctetViolins *pPlug, HISSTools_LICE_Vec_Lib *vecDraw, double x, double y, double w, double h, HISSTools_Design_Scheme *designScheme) : HISSTools_Button(mPlug, -1, vecDraw, x, y, w, h, "remove", designScheme), mPlug(pPlug)
{
    mName = "Delete";
}

void RemoveIRButton::OnMouseDown(int x, int y, IMouseMod* pMod)
{
    mValue = 1.0;
    SetDirty(false);
}

void RemoveIRButton::OnMouseDrag(int x, int y, int dX, int dY, IMouseMod* pMod)
{
    mValue = mTargetRECT.Contains(x, y);
    SetDirty(false);
}

void RemoveIRButton::OnMouseUp(int x, int y, IMouseMod* pMod)
{
    OnMouseDrag(x, y, 0, 0, pMod);
    
    if (mValue)
        mPlug->RemoveIR();
    
    mValue = 0.0;
    SetDirty();
}

// Preset Button

PresetButton::PresetButton::PresetButton(OctetViolins *pPlug, HISSTools_LICE_Vec_Lib *vecDraw, int idx, double x, double y, double w, double h, const char *type, HISSTools_Design_Scheme *designScheme) : HISSTools_Button(mPlug, -1, vecDraw, x, y, w, h, type, designScheme), mPlug(pPlug), mIdx(idx)
{
    mLabel.SetFormatted(32, "%d", idx + 1);
    mName = mLabel.Get();
    mMEWhenGrayed = true;
}

void PresetButton::OnMouseDown(int x, int y, IMouseMod* pMod)
{
    // N.B. The plug in deal with updating graphical state for all buttons
    
    if (pMod->S && !pMod->A && !pMod->C)
        mPlug->SavePreset(mIdx);
    else if (mGrayed)
        return;
    else if (pMod->A && pMod->C)
        mPlug->RemovePreset(mIdx);
    else
        mPlug->SetPreset(mIdx);
}

// File Save and Load Buttons

FileSaveLoad::FileSaveLoad(OctetViolins *pPlug, HISSTools_LICE_Vec_Lib *vecDraw, const char *label, double x, double y, double w, double h, EFileAction action, const char *type, HISSTools_Design_Scheme *designScheme)
: HISSTools_Button(pPlug, -1, vecDraw, x, y, w, h, type, designScheme), mFileAction(action), mPlug(pPlug)
{
    mName = label;
}

void FileSaveLoad::OnMouseDown(int x, int y, IMouseMod* pMod)
{
    WDL_String fileName;
    WDL_String dir;
    
    SetValueFromPlug(1.0);
    mPlug->GetGUI()->PromptForFile(&fileName, mFileAction, &dir, "octet");
    
    if (mFileAction == kFileOpen)
        mPlug->ReadState(fileName.Get());
    else
        mPlug->WriteState(fileName.Get());
    
    SetValueFromPlug(0.0);
}

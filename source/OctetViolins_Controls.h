
#ifndef OCTETVIOLINS_CONTROLS_H
#define OCTETVIOLINS_CONTROLS_H

#include "HISSTools_Controls.hpp"

class OctetViolins;

// Add IR Button

class AddIRButton : public HISSTools_Button
{
    
public:
    
    AddIRButton(OctetViolins *pPlug, HISSTools_VecLib *vecDraw, double x, double y, double w = 0, double h = 0, const char *type = 0, HISSTools_Design_Scheme *designScheme = &DefaultDesignScheme);
    
    virtual void OnMouseDown(int x, int y, IMouseMod* pMod);
    virtual void OnMouseUp(int x, int y, IMouseMod* pMod);
    
private:
    
    OctetViolins *mPlug;
};

// Remove IR Button

class RemoveIRButton : public HISSTools_Button
{
    
public:
    
    RemoveIRButton(OctetViolins *pPlug, HISSTools_VecLib *vecDraw, double x, double y, double w = 0, double h = 0, HISSTools_Design_Scheme *designScheme = &DefaultDesignScheme);
    
    virtual void OnMouseDown(int x, int y, IMouseMod* pMod);
    virtual void OnMouseDrag(int x, int y, int dX, int dY, IMouseMod* pMod);
    virtual void OnMouseUp(int x, int y, IMouseMod* pMod);
private:
    
    OctetViolins *mPlug;
};

// Preset Button

class PresetButton : public HISSTools_Button
{
    
public:
    
    PresetButton(OctetViolins *pPlug, HISSTools_VecLib *vecDraw, int idx, double x, double y, double w = 0, double h = 0, const char *type = 0, HISSTools_Design_Scheme *designScheme = &DefaultDesignScheme);
    
    virtual void OnMouseDown(int x, int y, IMouseMod* pMod);
    
private:
    
    OctetViolins *mPlug;
    int mIdx;
    WDL_String mLabel;
};

// File Save and Load Buttons

class FileSaveLoad : public HISSTools_Button
{
    
public:
    
    FileSaveLoad(OctetViolins *pPlug, HISSTools_VecLib *vecDraw, const char *label, double x, double y, double w, double h, EFileAction action, const char *type = 0, HISSTools_Design_Scheme *designScheme = &DefaultDesignScheme);
    
    virtual void OnMouseDown(int x, int y, IMouseMod* pMod);
    
private:
    
    OctetViolins *mPlug;
    EFileAction mFileAction;
};

#endif /* OCTETVIOLINS_CONTROLS_H */

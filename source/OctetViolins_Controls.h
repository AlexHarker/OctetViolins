
#ifndef OCTETVIOLINS_CONTROLS_H
#define OCTETVIOLINS_CONTROLS_H

#define NO_HISSTOOLS_CONTROL_HELPERS_COMPILE
#include "HISSTools_Controls.hpp"
#undef NO_HISSTOOLS_CONTROL_HELPERS_COMPILE

class OctetViolins;

// Add IR Button

class AddIRButton : public HISSTools_Button
{
public:
    
    AddIRButton(OctetViolins *pPlug, double x, double y, double w = 0, double h = 0, const char *type = 0, HISSTools_Design_Scheme *designScheme = &DefaultDesignScheme);
    
    void OnMouseDown(float x, float y, const IMouseMod& mod) override;
    void OnMouseUp(float x, float y, const IMouseMod& mod) override;
    
private:
    
    OctetViolins *mPlug;
};

// Remove IR Button

class RemoveIRButton : public HISSTools_Button
{
public:
    
    RemoveIRButton(OctetViolins *pPlug, double x, double y, double w = 0, double h = 0, HISSTools_Design_Scheme *designScheme = &DefaultDesignScheme);
    
    void OnMouseDown(float x, float y, const IMouseMod& mod) override;
    void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override;
    void OnMouseUp(float x, float y, const IMouseMod& mod) override;
    
private:
    
    OctetViolins *mPlug;
};

// Preset Button

class PresetButton : public HISSTools_Button
{
public:
    
    PresetButton(OctetViolins *pPlug, int idx, double x, double y, double w = 0, double h = 0, const char *type = 0, HISSTools_Design_Scheme *designScheme = &DefaultDesignScheme);
    
    void OnMouseDown(float x, float y, const IMouseMod& mod) override;
    
private:
    
    OctetViolins *mPlug;
    int mIdx;
};

// File Save and Load Buttons

class FileSaveLoad : public HISSTools_Button
{
public:
    
    FileSaveLoad(OctetViolins *pPlug, const char *label, double x, double y, double w, double h, EFileAction action, const char *type = 0, HISSTools_Design_Scheme *designScheme = &DefaultDesignScheme);
    
    void OnMouseDown(float x, float y, const IMouseMod& mod) override;
    
private:
    
    OctetViolins *mPlug;
    EFileAction mFileAction;
};

#endif /* OCTETVIOLINS_CONTROLS_H */

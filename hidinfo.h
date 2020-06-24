#ifndef HIDINFO_H
#define HIDINFO_H

#include "listbox.h"
#include "hid.h"
#include <iostream>

class HidDevice;

class HidInfo
{
public:
    std::string report(HID_DATA *data);
    void displayDeviceCaps(const HIDP_CAPS *pCaps, Listbox &lb) const;
    void displayButtonAttributes(HIDP_BUTTON_CAPS *pButton, Listbox &lb);
    void displayValueAttributes(HIDP_VALUE_CAPS *pValue, Listbox &lb) const;
    void displayDeviceAttributes(const HIDD_ATTRIBUTES *pAttrib, Listbox &lb) const;
    void displayInputButtons(HidDevice *pDevice, Listbox &lb) const;
    void displayInputValues(HidDevice *pDevice, Listbox &lb);
    void displayOutputButtons(HidDevice *pDevice, Listbox &lb);
    void displayOutputValues(HidDevice *pDevice, Listbox &lb);
    void displayFeatureButtons(HidDevice *pDevice, Listbox &lb) const;
    void displayFeatureValues(HidDevice *pDevice, Listbox &lb) const;
};

#endif


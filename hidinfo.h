#ifndef HIDINFO_H
#define HIDINFO_H

#include "listbox.h"
#include <strsafe.h>
#include <hidsdi.h>

class HidDevice;

class HidInfo
{
public:
    static LRESULT addStrToCtrl(HWND hCtrl, STRSAFE_LPCSTR pszFormat, ...);
    void displayDeviceCaps(HIDP_CAPS *pCaps, Listbox &lb);
    void displayButtonAttributes(HIDP_BUTTON_CAPS *pButton, HWND hControl);
    void displayValueAttributes(HIDP_VALUE_CAPS *pValue, Listbox &lb);
    void displayDeviceAttributes(HIDD_ATTRIBUTES *pAttrib, Listbox &lb);
    void displayInputButtons(HidDevice *pDevice, Listbox &lb);
    void displayInputValues(HidDevice *pDevice, Listbox &lb);
    void displayOutputButtons(HidDevice *pDevice, Listbox &lb);
    void displayOutputValues(HidDevice *pDevice, Listbox &lb);
    void displayFeatureButtons(HidDevice *pDevice, Listbox &lb);
    void displayFeatureValues(HidDevice *pDevice, Listbox &lb);
};

#endif


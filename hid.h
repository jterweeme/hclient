/*++

Copyright (c) Microsoft 1998, All Rights Reserved

Module Name:

    hid.h

Abstract:

    This module contains the declarations and definitions for use with the
    hid user mode client sample driver.

Environment:

    Kernel & user mode

--*/

#ifndef HID_H
#define HID_H

#include <windows.h>
#include <hidsdi.h>
#include <setupapi.h>
#include <iostream>

struct HID_DATA
{
    BOOLEAN IsButtonData;
    UCHAR Reserved;
    USAGE UsagePage;
    NTSTATUS Status;
    ULONG ReportID;
    BOOLEAN IsDataSet;

   union {
      struct {
         ULONG UsageMin;       // Variables to track the usage minimum and max
         ULONG UsageMax;       // If equal, then only a single usage
         ULONG MaxUsageLength; // Usages buffer length.
         USAGE *Usages;         // list of usages (buttons ``down'' on the device.

      } ButtonData;
      struct {
         USAGE Usage; // The usage describing this value;
         USHORT Reserved;

         ULONG Value;
         LONG ScaledValue;
      } ValueData;
   };
};

struct HID_DEVICE
{
    CHAR *OutputReportBuffer;
    HID_DATA *OutputData;
    ULONG OutputDataLength;
    HIDP_BUTTON_CAPS *OutputButtonCaps;
    HIDP_VALUE_CAPS *OutputValueCaps;

    CHAR *FeatureReportBuffer;
    HID_DATA *FeatureData;
    ULONG FeatureDataLength;
    HIDP_BUTTON_CAPS *FeatureButtonCaps;
    HIDP_VALUE_CAPS *FeatureValueCaps;
};

class HidDevice
{
private:
    HANDLE _handle;
    HID_DEVICE _dev;
    std::string _path;
    HIDD_ATTRIBUTES _Attributes;
    _HIDP_PREPARSED_DATA *_ppd;
    HIDP_CAPS _Caps;
    CHAR *InputReportBuffer;
    HID_DATA *InputData;
    ULONG InputDataLength;
    HIDP_BUTTON_CAPS *InputButtonCaps;
    HIDP_VALUE_CAPS *InputValueCaps;
    void _fillInputInfo();
    void _fillOutputInfo();
    void _fillFeatureInfo();
    VOID _CloseHidDevice(HID_DEVICE *HidDevice);
    BOOLEAN PackReport(HID_DATA *Data, ULONG DataLength);
    BOOLEAN _UnpackReport();
public:
    HidDevice();
    ~HidDevice();
    HID_DEVICE *getp();
    LPCSTR devicePath() const;
    HANDLE handle() const;
    const HIDD_ATTRIBUTES *attributes() const;
    HID_DATA *inputData() const;
    ULONG inputDataLen() const;
    USHORT vendorId() const;
    USHORT productId() const;
    const HIDP_CAPS *caps();
    const HIDP_BUTTON_CAPS *inputButtonCaps() const;
    const HIDP_VALUE_CAPS *inputValueCaps() const;
    BOOLEAN read();
    BOOLEAN write();
    BOOLEAN readOverlapped(HANDLE completionEv, LPOVERLAPPED overlap);
    void close();
    BOOLEAN open(LPCSTR path, BOOL read, BOOL write, BOOL overlapped, BOOL exclusive);
};

class HidButtonCaps
{
private:
    HIDP_BUTTON_CAPS _caps;
public:
    HidButtonCaps(HIDP_BUTTON_CAPS caps);
    BOOLEAN isRange() const;
    USAGE usageMin() const;
    USAGE usageMax() const;
};

#endif


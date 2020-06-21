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

struct SP_FNCLASS_DEVICE_DATA
{
    DWORD cbSize;
    GUID  FunctionClassGuid;
    TCHAR DevicePath [ANYSIZE_ARRAY];
};

// A structure to hold the steady state data received from the hid device.
// Each time a read packet is received we fill in this structure.
// Each time we wish to write to a hid device we fill in this structure.
// This structure is here only for convenience.  Most real applications will
// have a more efficient way of moving the hid data to the read, write, and
// feature routines.
struct HID_DATA
{
    BOOLEAN     IsButtonData;
    UCHAR       Reserved;
    USAGE       UsagePage;   // The usage page for which we are looking.
    ULONG       Status;      // The last status returned from the accessor function
                            // when updating this field.
    ULONG       ReportID;    // ReportID for this given data structure
    BOOLEAN     IsDataSet;   // Variable to track whether a given data structure
                            //  has already been added to a report structure

   union {
      struct {
         ULONG UsageMin;       // Variables to track the usage minimum and max
         ULONG UsageMax;       // If equal, then only a single usage
         ULONG MaxUsageLength; // Usages buffer length.
         PUSAGE Usages;         // list of usages (buttons ``down'' on the device.

      } ButtonData;
      struct {
         USAGE       Usage; // The usage describing this value;
         USHORT      Reserved;

         ULONG       Value;
         LONG        ScaledValue;
      } ValueData;
   };
};

struct HID_DEVICE
{
    CHAR *DevicePath;
    HANDLE HidDevice; // A file handle to the hid device.
    BOOL OpenedForRead;
    BOOL OpenedForWrite;
    BOOL OpenedOverlapped;
    BOOL OpenedExclusive;
    PHIDP_PREPARSED_DATA Ppd; // The opaque parser info describing this device
    HIDP_CAPS Caps; // The Capabilities of this hid device.
    HIDD_ATTRIBUTES Attributes;
    CHAR *InputReportBuffer;
    HID_DATA *InputData; // array of hid data structures
    ULONG InputDataLength; // Num elements in this array.
    HIDP_BUTTON_CAPS *InputButtonCaps;
    HIDP_VALUE_CAPS *InputValueCaps;
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

BOOLEAN UnpackReport(PCHAR ReportBuffer, USHORT ReportBufferLength,
   HIDP_REPORT_TYPE ReportType, HID_DATA *Data,
   ULONG DataLength, PHIDP_PREPARSED_DATA Ppd);

VOID CloseHidDevice(HID_DEVICE *HidDevice);

class HidDevice
{
private:
    HID_DEVICE _dev;
    BOOLEAN _FillDeviceInfo(HID_DEVICE *HidDevice);
    BOOLEAN HidWrite(HID_DEVICE *HidDevice);

    BOOLEAN PackReport(PCHAR ReportBuffer, USHORT ReportBufferLength,
                       HIDP_REPORT_TYPE ReportType, HID_DATA *Data,
                       ULONG DataLength, PHIDP_PREPARSED_DATA Ppd);
public:
    HidDevice();
    ~HidDevice();
    void set(HID_DEVICE dev);
    HID_DEVICE get() const;
    HID_DEVICE *getp();
    LPCSTR devicePath() const;
    HANDLE handle() const;

    BOOLEAN read();
    BOOLEAN write();
    BOOLEAN readOverlapped(HANDLE completionEv, LPOVERLAPPED overlap);
    void close();

    BOOLEAN open(LPCSTR path, BOOL hasReadAccess, BOOL hasWriteAccess,
                 BOOL isOverlapped, BOOL isExclusive);
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


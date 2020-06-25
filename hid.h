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

#include "hiditem.h"
#include <iostream>

class HidDevice
{
private:
    HANDLE _handle;
    std::string _path;
    HIDD_ATTRIBUTES _Attributes;
    _HIDP_PREPARSED_DATA *_ppd;
    HIDP_CAPS _Caps;
    HidInput _input;
    HidOutput _output;
    HidFeature _feature;
    BOOLEAN _packReport(HID_DATA *Data, ULONG DataLength);
    BOOLEAN _UnpackReport();
public:
    HidDevice();
    ~HidDevice();
    LPCSTR devicePath() const;
    HANDLE handle() const;
    HIDD_ATTRIBUTES const *attributes() const;
    HIDP_CAPS const *caps();
    HidItem const *input() const;
    HidItem const *output() const;
    HidItem const *feature() const;
    HID_DATA *inputData() const;
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


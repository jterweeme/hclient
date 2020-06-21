#ifndef HIDFINDER_H
#define HIDFINDER_H

#include "hid.h"

class PnPFinder
{
private:
    GUID _guid;
    HDEVINFO _hwDevInfo;
    SP_DEVICE_INTERFACE_DATA _devIfaceData;
    DWORD _i;
    BOOL _hasNext;
public:
    PnPFinder(GUID guid);
    BOOL hasNext();
    std::string next();
};

class HidFinder
{
    PnPFinder *_pnpFinder;
public:
    HidFinder();
    virtual ~HidFinder();
    BOOL hasNext();
    HidDevice *next();
};

#endif


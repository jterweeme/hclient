#include "hidfinder.h"

PnPFinder::PnPFinder(GUID guid) : _guid(guid), _i(0), _hasNext(FALSE)
{
    _hwDevInfo = ::SetupDiGetClassDevsA(&_guid, 0, 0, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    _devIfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
}

BOOL PnPFinder::hasNext()
{
    _hasNext = SetupDiEnumDeviceInterfaces(_hwDevInfo, NULL, &_guid, _i, &_devIfaceData);
    return _hasNext;
}

std::string PnPFinder::next()
{
    BOOL ret = TRUE;

    if (!_hasNext)
        ret = SetupDiEnumDeviceInterfaces(_hwDevInfo, NULL, &_guid, _i, &_devIfaceData);

    if (!ret)
        return std::string("");

    DWORD requiredLength = 0;
    SetupDiGetDeviceInterfaceDetailA(_hwDevInfo, &_devIfaceData, NULL, 0, &requiredLength, 0);
    DWORD predictedLength = requiredLength;
    SP_DEVICE_INTERFACE_DETAIL_DATA_A *devIfaceDetail;
    //devIfaceDetail = new SP_DEVICE_INTERFACE_DETAIL_DATA_A[requiredLength]
    devIfaceDetail = PSP_DEVICE_INTERFACE_DETAIL_DATA_A(malloc(requiredLength));
    devIfaceDetail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);
    ZeroMemory(devIfaceDetail->DevicePath, sizeof(devIfaceDetail->DevicePath));

    SetupDiGetDeviceInterfaceDetailA(_hwDevInfo, &_devIfaceData,
                      devIfaceDetail, predictedLength, &requiredLength, 0);

    std::string s(devIfaceDetail->DevicePath);
    _i++;
    _hasNext = FALSE;
    return s;
}

HidFinder::HidFinder()
{
    GUID hidGuid;
    HidD_GetHidGuid(&hidGuid);
    _pnpFinder = new PnPFinder(hidGuid);
}

HidFinder::~HidFinder()
{
    delete _pnpFinder;
}

BOOL HidFinder::hasNext()
{
    return _pnpFinder->hasNext();
}

HidDevice *HidFinder::next()
{
    std::string path = _pnpFinder->next();
    HidDevice *dev = new HidDevice;
    dev->open(path.c_str(), FALSE, FALSE, FALSE, FALSE);
    return dev;
}

HidButtonCaps::HidButtonCaps(HIDP_BUTTON_CAPS caps)
{
    _caps = caps;
}

BOOLEAN HidButtonCaps::isRange() const
{
    return _caps.IsRange;
}

USAGE HidButtonCaps::usageMin() const
{
    return _caps.Range.UsageMin;
}

USAGE HidButtonCaps::usageMax() const
{
    return _caps.Range.UsageMax;
}


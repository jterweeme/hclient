#include "hid.h"
#include <strsafe.h>

HidDevice::HidDevice()
{
}

HidDevice::~HidDevice()
{
}

LPCSTR HidDevice::devicePath() const
{
    return _path.c_str();
}

HANDLE HidDevice::handle() const
{
    return _handle;
}

HID_DATA *HidDevice::inputData() const
{
    return _input._data;
}

const HIDD_ATTRIBUTES *HidDevice::attributes() const
{
    return &_Attributes;
}

const HIDP_CAPS *HidDevice::caps()
{
    return &_Caps;
}

HidItem const *HidDevice::input() const
{
    return &_input;
}

HidItem const *HidDevice::output() const
{
    return &_output;
}

HidItem const *HidDevice::feature() const
{
    return &_feature;
}

void HidDevice::close()
{
}

BOOLEAN HidDevice::read()
{
    DWORD bytesRead;

    if (!ReadFile(handle(), _input._buffer, _Caps.InputReportByteLength, &bytesRead, NULL))
    {
        return FALSE;
    }

    if (bytesRead != _Caps.InputReportByteLength)
        return FALSE;

    return _UnpackReport();
}

BOOLEAN HidDevice::readOverlapped(HANDLE completionEv, LPOVERLAPPED overlap)
{
    memset(overlap, 0, sizeof(OVERLAPPED));
    overlap->hEvent = completionEv;
    DWORD bytesRead;
    BOOL readStatus = ReadFile(handle(), _input._buffer,
                         _Caps.InputReportByteLength, &bytesRead, overlap);

    if (!readStatus)
        return GetLastError() == ERROR_IO_PENDING;

    SetEvent(completionEv);
    return TRUE;
}

BOOLEAN HidDevice::_packReport(HID_DATA *Data, ULONG DataLength)
{
    char *ReportBuffer = _output._buffer;
    ULONG CurrReportID;
    memset(ReportBuffer, UCHAR(0), _output.bufLen());
    CurrReportID = Data->ReportID;

    for (ULONG i = 0; i < DataLength; i++, Data++)
    {
        if (Data->ReportID != CurrReportID)
            continue;

        if (Data->IsButtonData)
        {
            ULONG numUsages = Data->ButtonData.MaxUsageLength;
            Data->Status = HidP_SetUsages(HidP_Output, Data->UsagePage, 0,
                                Data->ButtonData.Usages, &numUsages, _ppd,
                                ReportBuffer, _output.bufLen());
        }
        else
        {
            Data->Status = HidP_SetUsageValue(HidP_Output, Data->UsagePage, 0,
                                 Data->ValueData.Usage, Data->ValueData.Value,
                                 _ppd, ReportBuffer, _output.bufLen());
        }

        if (Data->Status != HIDP_STATUS_SUCCESS)
            return FALSE;
    }

    for (ULONG i = 0; i < DataLength; i++, Data++)
        if (CurrReportID == Data->ReportID)
            Data->IsDataSet = TRUE;

    return TRUE;
}

BOOLEAN HidDevice::write()
{
    HID_DATA *pData = _output._data;

    for (ULONG i = 0; i < _output.dataLength(); i++, pData++)
        pData->IsDataSet = FALSE;

    BOOLEAN Status = TRUE;
    pData = _output._data;

    for (ULONG Index = 0; Index < _output.dataLength(); Index++, pData++)
    {
        if (pData->IsDataSet)
            continue;

        DWORD bytesWritten;
        _packReport(pData, _output.dataLength() - Index);

        BOOLEAN WriteStatus = WriteFile(handle(), _output._buffer,
                      _Caps.OutputReportByteLength, &bytesWritten, NULL);

        WriteStatus = WriteStatus && bytesWritten == _Caps.OutputReportByteLength;
        Status = Status && WriteStatus;
    }
    return Status;
}

BOOLEAN HidDevice::_UnpackReport()
{
    char *ReportBuffer = _input._buffer;
    USHORT ReportBufferLength = _Caps.InputReportByteLength;
    HIDP_REPORT_TYPE ReportType = HidP_Input;
    HID_DATA *Data = _input._data;
    ULONG DataLength = _input.dataLength();
    BOOLEAN result = FALSE;
    UCHAR reportID = ReportBuffer[0];

    for (ULONG i = 0; i < DataLength; i++, Data++)
    {
        if (reportID != Data->ReportID)
            continue;

        if (Data->IsButtonData)
        {
            ULONG numUsages = Data->ButtonData.MaxUsageLength;

            Data->Status = HidP_GetUsages(ReportType, Data->UsagePage, 0,
                                Data->ButtonData.Usages, &numUsages, _ppd,
                                ReportBuffer, ReportBufferLength);

            if (Data->Status != HIDP_STATUS_SUCCESS)
                return result;

            ULONG Index, nextUsage;
            for (Index = 0, nextUsage = 0; Index < numUsages; Index++)
            {
                if (Data->ButtonData.UsageMin <= Data->ButtonData.Usages[Index] &&
                        Data->ButtonData.Usages[Index] <= Data->ButtonData.UsageMax)
                {
                    Data->ButtonData.Usages[nextUsage++] = Data->ButtonData.Usages[Index];
                }
            }

            if (nextUsage < Data->ButtonData.MaxUsageLength)
                Data->ButtonData.Usages[nextUsage] = 0;
        }
        else
        {
            Data->Status = HidP_GetUsageValue(ReportType, Data->UsagePage, 0,
                             Data->ValueData.Usage, &Data->ValueData.Value, _ppd,
                             ReportBuffer, ReportBufferLength);

            if (Data->Status != HIDP_STATUS_SUCCESS)
                return result;

            Data->Status = HidP_GetScaledUsageValue(ReportType, Data->UsagePage, 0,
                                  Data->ValueData.Usage, &Data->ValueData.ScaledValue,
                                  _ppd, ReportBuffer, ReportBufferLength);

            if (Data->Status != HIDP_STATUS_SUCCESS && Data->Status != HIDP_STATUS_NULL)
            {
                return result;
            }
        }
        Data->IsDataSet = TRUE;
    }

    result = TRUE;
    return result;
}

BOOLEAN HidDevice::open(LPCSTR path, BOOL read, BOOL write,
    BOOL isOverlapped, BOOL isExclusive)
{
    DWORD accessFlags = 0;
    DWORD sharingFlags = 0;
    BOOLEAN bRet = FALSE;
    _handle = INVALID_HANDLE_VALUE;

    if (path == NULL)
        return FALSE;

    _path = std::string(path);

    if (read)
        accessFlags |= GENERIC_READ;

    if (write)
        accessFlags |= GENERIC_WRITE;

    if (!isExclusive)
        sharingFlags = FILE_SHARE_READ | FILE_SHARE_WRITE;

    _handle = CreateFileA(path, accessFlags, sharingFlags, NULL, OPEN_EXISTING, 0, NULL);

    if (handle() == INVALID_HANDLE_VALUE)
        goto Done;

    if (!HidD_GetPreparsedData(handle(), &_ppd))
        goto Done;

    if (!HidD_GetAttributes(handle(), &_Attributes))
        goto Done;

    if (!HidP_GetCaps(_ppd, &_Caps))
        goto Done;

    _input.init(&_Caps, _ppd);
    _output.init(&_Caps, _ppd);
    _feature.init(&_Caps, _ppd);

    if (isOverlapped)
    {
        CloseHandle(handle());
        _handle = INVALID_HANDLE_VALUE;

        _handle = CreateFileA(path, accessFlags, sharingFlags,
                       NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);

        if (handle() == INVALID_HANDLE_VALUE)
            goto Done;
    }

    bRet = TRUE;

Done:
    if (!bRet)
        close();

    return bRet;
}


#include "hid.h"
#include "toolbox.h"
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

HidData *HidDevice::inputData() const
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

HidInput const *HidDevice::input() const
{
    return &_input;
}

HidOutput const *HidDevice::output() const
{
    return &_output;
}

HidFeature const *HidDevice::feature() const
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

    return _unpackReport();
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

void HidDevice::_packReport(HidData *Data, ULONG DataLength)
{
    memset(_output._buffer, UCHAR(0), _output.bufLen());
    ULONG CurrReportID = Data[0].ReportID;

    for (ULONG i = 0; i < DataLength; i++)
    {
        if (Data[i].ReportID != CurrReportID)
            continue;

        if (Data[i].IsButtonData)
        {
            ULONG numUsages = Data[i].ButtonData.MaxUsageLength;
            Data[i].Status = HidP_SetUsages(HidP_Output, Data[i].UsagePage, 0,
                                Data[i].ButtonData.Usages, &numUsages, _ppd,
                                _output._buffer, _output.bufLen());
        }
        else
        {
            Data[i].Status = HidP_SetUsageValue(HidP_Output, Data[i].UsagePage, 0,
                                 Data[i].ValueData.Usage, Data[i].ValueData.Value,
                                 _ppd, _output._buffer, _output.bufLen());
        }

        if (Data[i].Status != HIDP_STATUS_SUCCESS)
            throw "SetUsages!";

        Data[i].IsDataSet = TRUE;
    }
}

BOOL HidDevice::write(const char *buf, USHORT n, LPDWORD bytesWritten)
{
    return WriteFile(handle(), buf, n, bytesWritten, nullptr);
}

BOOLEAN HidDevice::write()
{
    for (ULONG i = 0; i < _output.dataLength(); i++)
        _output._data[i].IsDataSet = FALSE;

    HidData *pData = _output._data;
    BOOLEAN Status = TRUE;

    for (ULONG i = 0; i < _output.dataLength(); i++, pData++)
    {
        if (pData->IsDataSet)
            continue;

        DWORD bytesWritten;
        _packReport(pData, _output.dataLength() - i);
        BOOLEAN WriteStatus = write(_output._buffer, _output.bufLen(), &bytesWritten);
        WriteStatus = WriteStatus && bytesWritten == _Caps.OutputReportByteLength;
        Status = Status && WriteStatus;
    }
    return Status;
}

BOOLEAN HidDevice::_unpackReport()
{
    HIDP_REPORT_TYPE ReportType = HidP_Input;
    HidData *Data = _input._data;
    ULONG DataLength = _input.dataLength();
    UCHAR reportID = _input._buffer[0];

    for (ULONG i = 0; i < DataLength; i++, Data++)
    {
        if (reportID != Data->ReportID)
            continue;

        if (Data->IsButtonData)
        {
            ULONG numUsages = Data->ButtonData.MaxUsageLength;

            Data->Status = HidP_GetUsages(ReportType, Data->UsagePage, 0,
                                Data->ButtonData.Usages, &numUsages, _ppd,
                                _input._buffer, _input.bufLen());

            if (Data->Status != HIDP_STATUS_SUCCESS)
                return FALSE;

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
                             _input._buffer, _input.bufLen());

            if (Data->Status != HIDP_STATUS_SUCCESS)
                return FALSE;

            Data->Status = HidP_GetScaledUsageValue(ReportType, Data->UsagePage, 0,
                                  Data->ValueData.Usage, &Data->ValueData.ScaledValue,
                                  _ppd, _input._buffer, _input.bufLen());

            if (Data->Status != HIDP_STATUS_SUCCESS && Data->Status != HIDP_STATUS_NULL)
            {
                return FALSE;
            }
        }
        Data->IsDataSet = TRUE;
    }

    return TRUE;
}

void HidDevice::open(LPCSTR path, BOOL read, BOOL write,
    BOOL isOverlapped, BOOL isExclusive)
{
    DWORD accessFlags = 0;
    DWORD sharingFlags = 0;
    BOOLEAN bRet = FALSE;
    _handle = INVALID_HANDLE_VALUE;

    if (path == NULL)
        throw "No path!";

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
    {
        close();
        throw "Error";
    }

}


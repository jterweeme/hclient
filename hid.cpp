#include "hid.h"
#include <strsafe.h>
#include <intsafe.h>
#include <iostream>

HidDevice::HidDevice()
{
    ZeroMemory(&_dev, sizeof(_dev));
}

HidDevice::~HidDevice()
{
}

HID_DEVICE *HidDevice::getp()
{
    return &_dev;
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
    return InputData;
}

ULONG HidDevice::inputDataLen() const
{
    return InputDataLength;
}

const HIDD_ATTRIBUTES *HidDevice::attributes() const
{
    return &_Attributes;
}

USHORT HidDevice::vendorId() const
{
    return attributes()->VendorID;
}

USHORT HidDevice::productId() const
{
    return attributes()->ProductID;
}

const HIDP_CAPS *HidDevice::caps()
{
    return &_Caps;
}

const HIDP_BUTTON_CAPS *HidDevice::inputButtonCaps() const
{
    return InputButtonCaps;
}

const HIDP_VALUE_CAPS *HidDevice::inputValueCaps() const
{
    return InputValueCaps;
}

void HidDevice::close()
{
    _CloseHidDevice(&_dev);
}

BOOLEAN HidDevice::read()
{
    DWORD bytesRead;

    if (!ReadFile(handle(), InputReportBuffer,
                  _Caps.InputReportByteLength, &bytesRead, NULL))
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
    BOOL readStatus = ReadFile(handle(), InputReportBuffer,
                         _Caps.InputReportByteLength, &bytesRead, overlap);

    if (!readStatus)
        return GetLastError() == ERROR_IO_PENDING;

    SetEvent(completionEv);
    return TRUE;
}

/*++
Routine Description:
   This routine takes in a list of HID_DATA structures (DATA) and builds
      in ReportBuffer the given report for all data values in the list that
      correspond to the report ID of the first item in the list.

   For every data structure in the list that has the same report ID as the first
      item in the list will be set in the report.  Every data item that is
      set will also have it's IsDataSet field marked with TRUE.

   A return value of FALSE indicates an unexpected error occurred when setting
      a given data value.  The caller should expect that assume that no values
      within the given data structure were set.

   A return value of TRUE indicates that all data values for the given report
      ID were set without error.
--*/
BOOLEAN HidDevice::PackReport(HID_DATA *Data, ULONG DataLength)
{
    char *ReportBuffer = _dev.OutputReportBuffer;
    USHORT ReportBufferLength = _Caps.OutputReportByteLength;
    HIDP_REPORT_TYPE ReportType = HidP_Output;
    PHIDP_PREPARSED_DATA Ppd = _ppd;
    ULONG numUsages;
    ULONG CurrReportID;
    BOOLEAN result = FALSE;
    memset(ReportBuffer, UCHAR(0), ReportBufferLength);
    CurrReportID = Data->ReportID;

    for (ULONG i = 0; i < DataLength; i++, Data++)
    {
        if (Data->ReportID != CurrReportID)
            continue;

        if (Data->IsButtonData)
        {
            numUsages = Data->ButtonData.MaxUsageLength;
            Data->Status = HidP_SetUsages(ReportType, Data->UsagePage, 0,
                                Data->ButtonData.Usages, &numUsages, Ppd,
                                ReportBuffer, ReportBufferLength);
        }
        else
        {
            Data->Status = HidP_SetUsageValue(ReportType, Data->UsagePage, 0,
                                 Data->ValueData.Usage, Data->ValueData.Value,
                                 Ppd, ReportBuffer, ReportBufferLength);
        }

        if (HIDP_STATUS_SUCCESS != Data->Status)
            return result;
    }

    for (ULONG i = 0; i < DataLength; i++, Data++)
        if (CurrReportID == Data->ReportID)
            Data->IsDataSet = TRUE;

    result = TRUE;
    return result;
}

BOOLEAN HidDevice::write()
{
    HID_DATA *pData = _dev.OutputData;

    for (ULONG i = 0; i < _dev.OutputDataLength; i++, pData++)
        pData->IsDataSet = FALSE;

    BOOLEAN Status = TRUE;
    pData = _dev.OutputData;

    for (ULONG Index = 0; Index < _dev.OutputDataLength; Index++, pData++)
    {
        if (pData->IsDataSet)
            continue;

        DWORD bytesWritten;
        PackReport(pData, _dev.OutputDataLength - Index);

        BOOLEAN WriteStatus = WriteFile(handle(), _dev.OutputReportBuffer,
                      _Caps.OutputReportByteLength, &bytesWritten, NULL);

        WriteStatus = WriteStatus && bytesWritten == _Caps.OutputReportByteLength;
        Status = Status && WriteStatus;
    }
    return Status;
}

BOOLEAN HidDevice::_UnpackReport()
{
    char *ReportBuffer = InputReportBuffer;
    USHORT ReportBufferLength = _Caps.InputReportByteLength;
    HIDP_REPORT_TYPE ReportType = HidP_Input;
    HID_DATA *Data = InputData;
    ULONG DataLength = InputDataLength;
    PHIDP_PREPARSED_DATA Ppd = _ppd;
    ULONG numUsages; // Number of usages returned from GetUsages.
    ULONG Index;
    ULONG nextUsage;
    BOOLEAN result = FALSE;
    UCHAR reportID = ReportBuffer[0];

    for (ULONG i = 0; i < DataLength; i++, Data++)
    {
        if (reportID != Data->ReportID)
            continue;

        if (Data->IsButtonData)
        {
            numUsages = Data->ButtonData.MaxUsageLength;

            Data->Status = HidP_GetUsages(ReportType, Data->UsagePage, 0,
                                Data->ButtonData.Usages, &numUsages, Ppd,
                                ReportBuffer, ReportBufferLength);

            if (HIDP_STATUS_SUCCESS != Data->Status)
                return result;

            for (Index = 0, nextUsage = 0; Index < numUsages; Index++)
            {
                if (Data->ButtonData.UsageMin <= Data->ButtonData.Usages[Index] &&
                        Data->ButtonData.Usages[Index] <= Data->ButtonData.UsageMax)
                {
                    Data -> ButtonData.Usages[nextUsage++] = Data -> ButtonData.Usages[Index];
                }
            }

            if (nextUsage < Data->ButtonData.MaxUsageLength)
                Data->ButtonData.Usages[nextUsage] = 0;
        }
        else
        {
            Data->Status = HidP_GetUsageValue(ReportType, Data->UsagePage, 0,
                             Data->ValueData.Usage, &Data->ValueData.Value, Ppd,
                             ReportBuffer, ReportBufferLength);

            if (Data->Status != HIDP_STATUS_SUCCESS)
                return result;

            Data->Status = HidP_GetScaledUsageValue(ReportType, Data->UsagePage, 0,
                                  Data->ValueData.Usage, &Data->ValueData.ScaledValue,
                                  Ppd, ReportBuffer, ReportBufferLength);

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

void HidDevice::_fillInputInfo()
{
    InputReportBuffer = new char[_Caps.InputReportByteLength];
    InputButtonCaps = new HIDP_BUTTON_CAPS[_Caps.NumberInputButtonCaps];
    InputValueCaps = new HIDP_VALUE_CAPS[_Caps.NumberInputValueCaps];

    if (_Caps.NumberInputButtonCaps > 0)
    {
        if (HidP_GetButtonCaps(HidP_Input, InputButtonCaps,
                     &_Caps.NumberInputButtonCaps, _ppd) != HIDP_STATUS_SUCCESS)
        {
            throw "GetButtonCaps";
        }
    }

    if (_Caps.NumberInputValueCaps > 0)
    {
        if (HidP_GetValueCaps(HidP_Input, InputValueCaps,
                  &_Caps.NumberInputValueCaps, _ppd) != HIDP_STATUS_SUCCESS)
        {
            throw "GetValueCaps";
        }
    }

    ULONG inpNumValues = 0;
    for (ULONG i = 0; i < _Caps.NumberInputValueCaps; i++)
    {
        if (InputValueCaps[i].IsRange)
        {
            inpNumValues += InputValueCaps[i].Range.UsageMax - InputValueCaps[i].Range.UsageMin + 1;

            if (InputValueCaps[i].Range.UsageMin > InputValueCaps[i].Range.UsageMax)
                throw "Range incorrect";
        }
        else
        {
            inpNumValues++;
        }
    }

    HIDP_VALUE_CAPS *inpValueCaps = InputValueCaps;
    InputDataLength = _Caps.NumberInputButtonCaps + inpNumValues;
    InputData = new HID_DATA[InputDataLength];
    HID_DATA *data = InputData;
    UINT dataIdx = 0;
    HIDP_BUTTON_CAPS *inpButtonCaps = InputButtonCaps;

    for (ULONG i = 0; i < _Caps.NumberInputButtonCaps;
         i++, data++, inpButtonCaps++, dataIdx++)
    {
        data->IsButtonData = TRUE;
        data->Status = HIDP_STATUS_SUCCESS;
        data->UsagePage = inpButtonCaps->UsagePage;

        if (inpButtonCaps->IsRange)
        {
            data->ButtonData.UsageMin = inpButtonCaps->Range.UsageMin;
            data->ButtonData.UsageMax = inpButtonCaps->Range.UsageMax;
        }
        else
        {
            data->ButtonData.UsageMin = data->ButtonData.UsageMax = inpButtonCaps->NotRange.Usage;
        }

        data->ButtonData.MaxUsageLength = HidP_MaxUsageListLength(
                    HidP_Input, inpButtonCaps->UsagePage, _ppd);

        data->ButtonData.Usages = new USAGE[data->ButtonData.MaxUsageLength];
        data->ReportID = inpButtonCaps->ReportID;
    }

    for (ULONG i = 0; i < _Caps.NumberInputValueCaps ; i++, inpValueCaps++)
    {
        if (inpValueCaps->IsRange)
        {
            for (USAGE usage = inpValueCaps->Range.UsageMin;
                 usage <= inpValueCaps->Range.UsageMax;
                 usage++)
            {
                if (dataIdx >= InputDataLength)
                    throw "Out of bounds!";

                data->IsButtonData = FALSE;
                data->Status = HIDP_STATUS_SUCCESS;
                data->UsagePage = inpValueCaps->UsagePage;
                data->ValueData.Usage = usage;
                data->ReportID = inpValueCaps -> ReportID;
                data++;
                dataIdx++;
            }
        }
        else
        {
            if (dataIdx >= InputDataLength)
                throw "Out of bounds!";

            data->IsButtonData = FALSE;
            data->Status = HIDP_STATUS_SUCCESS;
            data->UsagePage = inpValueCaps->UsagePage;
            data->ValueData.Usage = inpValueCaps->NotRange.Usage;
            data->ReportID = inpValueCaps->ReportID;
            data++;
            dataIdx++;
        }
    }
}

void HidDevice::_fillOutputInfo()
{
    _dev.OutputReportBuffer = new char[_Caps.OutputReportByteLength];
    _dev.OutputButtonCaps = new HIDP_BUTTON_CAPS[_Caps.NumberOutputButtonCaps];
    _dev.OutputValueCaps = new HIDP_VALUE_CAPS[_Caps.NumberOutputValueCaps];

    if (_Caps.NumberOutputButtonCaps > 0)
    {
        if (HidP_GetButtonCaps(HidP_Output, _dev.OutputButtonCaps,
                     &_Caps.NumberOutputButtonCaps, _ppd) != HIDP_STATUS_SUCCESS)
        {
            throw "GetButtonCaps";
        }
    }

    if (_Caps.NumberOutputValueCaps > 0)
    {
        if (HidP_GetValueCaps(HidP_Output, _dev.OutputValueCaps,
                     &_Caps.NumberOutputValueCaps, _ppd) != HIDP_STATUS_SUCCESS)
        {
            throw "GetValueCaps";
        }
    }

    ULONG numValues = 0;
    for (ULONG i = 0; i < _Caps.NumberOutputValueCaps; i++)
    {
        if (_dev.OutputValueCaps[i].IsRange)
        {
            numValues += _dev.OutputValueCaps[i].Range.UsageMax - _dev.OutputValueCaps[i].Range.UsageMin + 1;
        }
        else
        {
            numValues++;
        }
    }

    HIDP_VALUE_CAPS *valueCaps = _dev.OutputValueCaps;
    _dev.OutputDataLength = _Caps.NumberOutputButtonCaps + numValues;
    _dev.OutputData = new HID_DATA[_dev.OutputDataLength];
    HID_DATA *outData = _dev.OutputData;

    ULONG tmpSum;
    HIDP_BUTTON_CAPS *buttonCaps = _dev.OutputButtonCaps;
    for (ULONG i = 0; i < _Caps.NumberOutputButtonCaps; i++, outData++, buttonCaps++)
    {
        if (i >= _dev.OutputDataLength)
        {
            throw "Out of bounds!";
        }

        if (ULongAdd(_Caps.NumberOutputButtonCaps, valueCaps->Range.UsageMax, &tmpSum) < 0)
        {
            throw "Out of bounds!";
        }

        if (valueCaps->Range.UsageMin == tmpSum)
            throw "Error";

        outData->IsButtonData = TRUE;
        outData->Status = HIDP_STATUS_SUCCESS;
        outData->UsagePage = buttonCaps->UsagePage;

        if (buttonCaps->IsRange)
        {
            outData->ButtonData.UsageMin = buttonCaps->Range.UsageMin;
            outData->ButtonData.UsageMax = buttonCaps->Range.UsageMax;
        }
        else
        {
            outData->ButtonData.UsageMin = outData->ButtonData.UsageMax = buttonCaps->NotRange.Usage;
        }

        outData->ButtonData.MaxUsageLength = HidP_MaxUsageListLength(
                        HidP_Output, buttonCaps->UsagePage, _ppd);

        outData->ButtonData.Usages = new USAGE[outData->ButtonData.MaxUsageLength];
        outData->ReportID = buttonCaps->ReportID;
    }

    for (ULONG i = 0; i < _Caps.NumberOutputValueCaps; i++, valueCaps++)
    {
        if (valueCaps->IsRange)
        {
            for (USAGE usage = valueCaps->Range.UsageMin;
                 usage <= valueCaps->Range.UsageMax;
                 usage++)
            {
                outData->IsButtonData = FALSE;
                outData->Status = HIDP_STATUS_SUCCESS;
                outData->UsagePage = valueCaps->UsagePage;
                outData->ValueData.Usage = usage;
                outData->ReportID = valueCaps -> ReportID;
                outData++;
            }
        }
        else
        {
            outData->IsButtonData = FALSE;
            outData->Status = HIDP_STATUS_SUCCESS;
            outData->UsagePage = valueCaps->UsagePage;
            outData->ValueData.Usage = valueCaps->NotRange.Usage;
            outData->ReportID = valueCaps -> ReportID;
            outData++;
        }
    }
}

void HidDevice::_fillFeatureInfo()
{
    void *tmp;
    _dev.FeatureReportBuffer = new char[_Caps.FeatureReportByteLength];
    _dev.FeatureButtonCaps = new HIDP_BUTTON_CAPS[_Caps.NumberFeatureButtonCaps];
    HIDP_BUTTON_CAPS *buttonCaps = _dev.FeatureButtonCaps;
    tmp = calloc(_Caps.NumberFeatureValueCaps, sizeof (HIDP_VALUE_CAPS));
    HIDP_VALUE_CAPS *valueCaps;
    _dev.FeatureValueCaps = valueCaps = PHIDP_VALUE_CAPS(tmp);
    USHORT numCaps = _Caps.NumberFeatureButtonCaps;

    if (numCaps > 0)
    {
        if (HIDP_STATUS_SUCCESS != HidP_GetButtonCaps(HidP_Feature,
                            buttonCaps, &numCaps, _ppd))
        {
            throw "GetButtonCaps";
        }
    }

    numCaps = _Caps.NumberFeatureValueCaps;

    if (numCaps > 0)
    {
        if (HIDP_STATUS_SUCCESS != HidP_GetValueCaps(HidP_Feature,
                           valueCaps, &numCaps, _ppd))
        {
            throw "GetValueCaps";
        }
    }

    ULONG numValues = 0;
    for (ULONG i = 0; i < _Caps.NumberFeatureValueCaps; i++, valueCaps++)
    {
        if (valueCaps->IsRange)
        {
            numValues += valueCaps->Range.UsageMax - valueCaps->Range.UsageMin + 1;
        }
        else
        {
            numValues++;
        }
    }
    valueCaps = _dev.FeatureValueCaps;
    ULONG newFeatureDataLength = 0;

    if (ULongAdd(_Caps.NumberFeatureButtonCaps,
                   numValues, &newFeatureDataLength) < 0)
    {
        throw "Error";
    }

    _dev.FeatureDataLength = newFeatureDataLength;
    tmp = calloc(_dev.FeatureDataLength, sizeof(HID_DATA));
    HID_DATA *data;
    _dev.FeatureData = data = (HID_DATA *)(tmp);
    UINT dataIdx = 0;
    for (ULONG i = 0; i < _Caps.NumberFeatureButtonCaps;
         i++, data++, buttonCaps++, dataIdx++)
    {
        data->IsButtonData = TRUE;
        data->Status = HIDP_STATUS_SUCCESS;
        data->UsagePage = buttonCaps->UsagePage;

        if (buttonCaps->IsRange)
        {
            data->ButtonData.UsageMin = buttonCaps->Range.UsageMin;
            data->ButtonData.UsageMax = buttonCaps->Range.UsageMax;
        }
        else
        {
            data->ButtonData.UsageMin = data->ButtonData.UsageMax = buttonCaps->NotRange.Usage;
        }

        data->ButtonData.MaxUsageLength = HidP_MaxUsageListLength(
                   HidP_Feature, buttonCaps->UsagePage, _ppd);

        data->ButtonData.Usages = new USAGE[data->ButtonData.MaxUsageLength];
        data->ReportID = buttonCaps->ReportID;
    }

    for (ULONG i = 0; i < _Caps.NumberFeatureValueCaps ; i++, valueCaps++)
    {
        if (valueCaps->IsRange)
        {
            for (USAGE usage = valueCaps->Range.UsageMin;
                 usage <= valueCaps->Range.UsageMax;
                 usage++)
            {
                if (dataIdx >= _dev.FeatureDataLength)
                    throw "Out of bounds!";

                data->IsButtonData = FALSE;
                data->Status = HIDP_STATUS_SUCCESS;
                data->UsagePage = valueCaps->UsagePage;
                data->ValueData.Usage = usage;
                data->ReportID = valueCaps->ReportID;
                data++;
                dataIdx++;
            }
        }
        else
        {
            if (dataIdx >= _dev.FeatureDataLength)
                throw "Out of bounds!";

            data->IsButtonData = FALSE;
            data->Status = HIDP_STATUS_SUCCESS;
            data->UsagePage = valueCaps->UsagePage;
            data->ValueData.Usage = valueCaps->NotRange.Usage;
            data->ReportID = valueCaps->ReportID;
            data++;
            dataIdx++;
        }
    }
}

BOOLEAN HidDevice::open(LPCSTR path, BOOL read, BOOL write,
    BOOL isOverlapped, BOOL isExclusive)
{
    DWORD accessFlags = 0;
    DWORD sharingFlags = 0;
    BOOLEAN bRet = FALSE;
    RtlZeroMemory(&_dev, sizeof(HID_DEVICE));
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

    _fillInputInfo();
    _fillOutputInfo();
    _fillFeatureInfo();

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

VOID HidDevice::_CloseHidDevice(HID_DEVICE *HidDevice)
{
    if (handle() != INVALID_HANDLE_VALUE)
    {
        CloseHandle(_handle);
        _handle = INVALID_HANDLE_VALUE;
    }
#if 0
    if (_ppd != NULL)
    {
        HidD_FreePreparsedData(_ppd);
        _ppd = NULL;
    }

    if (NULL != InputReportBuffer)
    {
        free(InputReportBuffer);
        InputReportBuffer = NULL;
    }

    if (NULL != HidDevice -> InputData)
    {
        free(HidDevice -> InputData);
        HidDevice -> InputData = NULL;
    }

    if (NULL != HidDevice -> InputButtonCaps)
    {
        free(HidDevice->InputButtonCaps);
        HidDevice->InputButtonCaps = NULL;
    }

    if (NULL != HidDevice -> InputValueCaps)
    {
        free(HidDevice -> InputValueCaps);
        HidDevice -> InputValueCaps = NULL;
    }
#endif
    if (NULL != HidDevice -> OutputReportBuffer)
    {
        free(HidDevice -> OutputReportBuffer);
        HidDevice -> OutputReportBuffer = NULL;
    }

    if (NULL != HidDevice -> OutputData)
    {
        free(HidDevice -> OutputData);
        HidDevice -> OutputData = NULL;
    }

    if (NULL != HidDevice -> OutputButtonCaps)
    {
        free(HidDevice -> OutputButtonCaps);
        HidDevice -> OutputButtonCaps = NULL;
    }

    if (NULL != HidDevice -> OutputValueCaps)
    {
        free(HidDevice -> OutputValueCaps);
        HidDevice -> OutputValueCaps = NULL;
    }

    if (NULL != HidDevice -> FeatureReportBuffer)
    {
        free(HidDevice -> FeatureReportBuffer);
        HidDevice -> FeatureReportBuffer = NULL;
    }

    if (NULL != HidDevice -> FeatureData)
    {
        free(HidDevice -> FeatureData);
        HidDevice -> FeatureData = NULL;
    }

    if (NULL != HidDevice->FeatureButtonCaps)
    {
        free(HidDevice -> FeatureButtonCaps);
        HidDevice -> FeatureButtonCaps = NULL;
    }

    if (NULL != HidDevice->FeatureValueCaps)
    {
        free(HidDevice->FeatureValueCaps);
        HidDevice->FeatureValueCaps = NULL;
    }
}


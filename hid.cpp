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

#if 1
void HidDevice::set(HID_DEVICE dev)
{
    _dev = dev;
}
#endif

#if 1
HID_DEVICE HidDevice::get() const
{
    return _dev;
}
#endif

HID_DEVICE *HidDevice::getp()
{
    return &_dev;
}

LPCSTR HidDevice::devicePath() const
{
    return _dev.DevicePath;
}

HANDLE HidDevice::handle() const
{
    return _handle;
}

HID_DATA *HidDevice::inputData() const
{
    return _dev.InputData;
}

ULONG HidDevice::inputDataLen() const
{
    return _dev.InputDataLength;
}

void HidDevice::close()
{
    _CloseHidDevice(&_dev);
}

BOOLEAN HidDevice::read()
{
    DWORD bytesRead;

    if (!ReadFile(handle(), _dev.InputReportBuffer,
                  _dev.Caps.InputReportByteLength, &bytesRead, NULL))
    {
        return FALSE;
    }

    if (bytesRead != _dev.Caps.InputReportByteLength)
        return FALSE;

    return UnpackReport(_dev.InputReportBuffer, _dev.Caps.InputReportByteLength,
                  HidP_Input, _dev.InputData, _dev.InputDataLength, _dev.Ppd);
}

BOOLEAN HidDevice::readOverlapped(HANDLE completionEv, LPOVERLAPPED overlap)
{
    memset(overlap, 0, sizeof(OVERLAPPED));
    overlap->hEvent = completionEv;
    DWORD bytesRead;
    BOOL readStatus = ReadFile(handle(), _dev.InputReportBuffer,
                         _dev.Caps.InputReportByteLength, &bytesRead, overlap);

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
BOOLEAN HidDevice::PackReport(PCHAR ReportBuffer, USHORT ReportBufferLength,
                   HIDP_REPORT_TYPE ReportType, HID_DATA *Data,
                   ULONG DataLength, PHIDP_PREPARSED_DATA Ppd)
{
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
        PackReport(_dev.OutputReportBuffer, _dev.Caps.OutputReportByteLength,
                   HidP_Output, pData, _dev.OutputDataLength - Index, _dev.Ppd);

        BOOLEAN WriteStatus = WriteFile(handle(), _dev.OutputReportBuffer,
                      _dev.Caps.OutputReportByteLength, &bytesWritten, NULL);

        WriteStatus = WriteStatus && bytesWritten == _dev.Caps.OutputReportByteLength;
        Status = Status && WriteStatus;
    }
    return Status;
}

/*++
Routine Description:
   Given ReportBuffer representing a report from a HID device where the first
   byte of the buffer is the report ID for the report, extract all the HID_DATA
   in the Data list from the given report.
--*/
BOOLEAN UnpackReport(PCHAR ReportBuffer, USHORT ReportBufferLength,
    HIDP_REPORT_TYPE ReportType, HID_DATA *Data,
    ULONG DataLength, PHIDP_PREPARSED_DATA Ppd)
{
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
                                            Data->ValueData.Usage,
                                            &Data->ValueData.Value, Ppd,
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

BOOLEAN HidDevice::_FillDeviceInfo()
{
    HID_DEVICE *HidDevice = &_dev;
    USAGE usage;
    UINT dataIdx;
    ULONG newFeatureDataLength = 0;
    ULONG tmpSum;
    BOOLEAN bRet = FALSE;
    HidDevice->InputReportBuffer = new char[HidDevice->Caps.InputReportByteLength];
    HIDP_BUTTON_CAPS *buttonCaps = new HIDP_BUTTON_CAPS[HidDevice->Caps.NumberInputButtonCaps];
    HidDevice->InputButtonCaps = buttonCaps;

    if (HidDevice->InputButtonCaps == NULL)
        return bRet;

    HIDP_VALUE_CAPS *valueCaps = new HIDP_VALUE_CAPS[HidDevice->Caps.NumberInputValueCaps];
    HidDevice->InputValueCaps = valueCaps;

    if (valueCaps == NULL)
        return bRet;

    USHORT numCaps = HidDevice->Caps.NumberInputButtonCaps;

    if (numCaps > 0)
    {
        if (HidP_GetButtonCaps(HidP_Input, HidDevice->InputButtonCaps,
                               &numCaps, HidDevice->Ppd) != HIDP_STATUS_SUCCESS)
        {
            return bRet;
        }
    }

    numCaps = HidDevice->Caps.NumberInputValueCaps;

    if (numCaps > 0)
        if (HidP_GetValueCaps(HidP_Input, valueCaps, &numCaps, HidDevice->Ppd) != HIDP_STATUS_SUCCESS)
            return bRet;

    ULONG numValues = 0;
    ULONG i;
    for (i = 0; i < HidDevice->Caps.NumberInputValueCaps; i++, valueCaps++)
    {
        if (valueCaps->IsRange)
        {
            numValues += valueCaps->Range.UsageMax - valueCaps->Range.UsageMin + 1;

            if (valueCaps->Range.UsageMin > valueCaps->Range.UsageMax)
                return bRet;  // overrun check
        }
        else
        {
            numValues++;
        }
    }

    valueCaps = HidDevice->InputValueCaps;
    HidDevice->InputDataLength = HidDevice->Caps.NumberInputButtonCaps + numValues;

    void *tmp;
    HID_DATA *data = new HID_DATA[HidDevice->InputDataLength];
    HidDevice->InputData = data;

    if (data == NULL)
        return bRet;

    // Fill in the button data
    dataIdx = 0;
    for (i = 0;
         i < HidDevice->Caps.NumberInputButtonCaps;
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
            data -> ButtonData.UsageMin = data->ButtonData.UsageMax = buttonCaps->NotRange.Usage;
        }

        data->ButtonData.MaxUsageLength = HidP_MaxUsageListLength(
                    HidP_Input, buttonCaps->UsagePage, HidDevice->Ppd);

        data->ButtonData.Usages = new USAGE[data->ButtonData.MaxUsageLength];
        data->ReportID = buttonCaps->ReportID;
    }

    // Fill in the value data
    for (i = 0; i < HidDevice->Caps.NumberInputValueCaps ; i++, valueCaps++)
    {
        if (valueCaps->IsRange)
        {
            for (usage = valueCaps->Range.UsageMin;
                 usage <= valueCaps->Range.UsageMax;
                 usage++)
            {
                if (dataIdx >= HidDevice->InputDataLength)
                    return bRet; // error case

                data->IsButtonData = FALSE;
                data->Status = HIDP_STATUS_SUCCESS;
                data->UsagePage = valueCaps->UsagePage;
                data->ValueData.Usage = usage;
                data->ReportID = valueCaps -> ReportID;
                data++;
                dataIdx++;
            }
        }
        else
        {
            if (dataIdx >= HidDevice->InputDataLength)
                return bRet;

            data->IsButtonData = FALSE;
            data->Status = HIDP_STATUS_SUCCESS;
            data->UsagePage = valueCaps->UsagePage;
            data->ValueData.Usage = valueCaps->NotRange.Usage;
            data->ReportID = valueCaps -> ReportID;
            data++;
            dataIdx++;
        }
    }

    // setup Output Data buffers.
    HidDevice->OutputReportBuffer = new char[HidDevice->Caps.OutputReportByteLength];
    tmp = calloc(HidDevice->Caps.NumberOutputButtonCaps, sizeof(HIDP_BUTTON_CAPS));
    HidDevice->OutputButtonCaps = buttonCaps = PHIDP_BUTTON_CAPS(tmp);

    if (NULL == buttonCaps)
        return bRet;

    tmp = calloc(HidDevice->Caps.NumberOutputValueCaps, sizeof (HIDP_VALUE_CAPS));
    HidDevice->OutputValueCaps = valueCaps = reinterpret_cast<HIDP_VALUE_CAPS *>(tmp);

    if (NULL == valueCaps)
        return bRet;

    numCaps = HidDevice->Caps.NumberOutputButtonCaps;

    if (numCaps > 0)
    {
        if ((HidP_GetButtonCaps(HidP_Output, buttonCaps, &numCaps,
                        HidDevice->Ppd)) != HIDP_STATUS_SUCCESS)
        {
            return bRet;
        }
    }

    numCaps = HidDevice->Caps.NumberOutputValueCaps;

    if (numCaps > 0)
    {
        if ((HidP_GetValueCaps(HidP_Output, valueCaps, &numCaps,
                      HidDevice->Ppd)) != HIDP_STATUS_SUCCESS)
        {
            return bRet;
        }
    }

    numValues = 0;

    for (i = 0; i < HidDevice->Caps.NumberOutputValueCaps; i++, valueCaps++)
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

    valueCaps = HidDevice->OutputValueCaps;
    HidDevice->OutputDataLength = HidDevice->Caps.NumberOutputButtonCaps + numValues;
    tmp = calloc(HidDevice->OutputDataLength, sizeof (HID_DATA));
    HidDevice->OutputData = data = reinterpret_cast<HID_DATA *>(tmp);

    if (data == NULL)
        return bRet;

    for (i = 0; i < HidDevice->Caps.NumberOutputButtonCaps; i++, data++, buttonCaps++)
    {
        if (i >= HidDevice->OutputDataLength)
        {
            return bRet;
        }

        if (FAILED(ULongAdd((HidDevice->Caps).NumberOutputButtonCaps ,
                                     (valueCaps->Range).UsageMax, &tmpSum)))
        {
            return bRet;
        }

        if ((valueCaps->Range).UsageMin == tmpSum)
            return bRet;

        data->IsButtonData = TRUE;
        data->Status = HIDP_STATUS_SUCCESS;
        data->UsagePage = buttonCaps->UsagePage;

        if (buttonCaps->IsRange)
        {
            data->ButtonData.UsageMin = buttonCaps -> Range.UsageMin;
            data->ButtonData.UsageMax = buttonCaps -> Range.UsageMax;
        }
        else
        {
            data->ButtonData.UsageMin = data->ButtonData.UsageMax = buttonCaps->NotRange.Usage;
        }

        data->ButtonData.MaxUsageLength = HidP_MaxUsageListLength(
                        HidP_Output, buttonCaps->UsagePage, HidDevice->Ppd);

        tmp = calloc(data->ButtonData.MaxUsageLength, sizeof (USAGE));
        data->ButtonData.Usages = (PUSAGE)tmp;
        data->ReportID = buttonCaps->ReportID;
    }

    for (i = 0; i < HidDevice->Caps.NumberOutputValueCaps; i++, valueCaps++)
    {
        if (valueCaps->IsRange)
        {
            for (usage = valueCaps->Range.UsageMin;
                 usage <= valueCaps->Range.UsageMax;
                 usage++)
            {
                data->IsButtonData = FALSE;
                data->Status = HIDP_STATUS_SUCCESS;
                data->UsagePage = valueCaps->UsagePage;
                data->ValueData.Usage = usage;
                data->ReportID = valueCaps -> ReportID;
                data++;
            }
        }
        else
        {
            data->IsButtonData = FALSE;
            data->Status = HIDP_STATUS_SUCCESS;
            data->UsagePage = valueCaps->UsagePage;
            data->ValueData.Usage = valueCaps->NotRange.Usage;
            data->ReportID = valueCaps -> ReportID;
            data++;
        }
    }

    // setup Feature Data buffers.
    tmp = calloc(HidDevice->Caps.FeatureReportByteLength, sizeof (CHAR));
    HidDevice->FeatureReportBuffer = (PCHAR)tmp;
    tmp = calloc(HidDevice->Caps.NumberFeatureButtonCaps, sizeof(HIDP_BUTTON_CAPS));
    HidDevice->FeatureButtonCaps = buttonCaps = PHIDP_BUTTON_CAPS(tmp);

    if (NULL == buttonCaps)
        return bRet;

    tmp = calloc(HidDevice->Caps.NumberFeatureValueCaps, sizeof (HIDP_VALUE_CAPS));
    HidDevice->FeatureValueCaps = valueCaps = PHIDP_VALUE_CAPS(tmp);

    if (NULL == valueCaps)
        return bRet;

    numCaps = HidDevice->Caps.NumberFeatureButtonCaps;

    if (numCaps > 0)
    {
        if (HIDP_STATUS_SUCCESS != (HidP_GetButtonCaps(HidP_Feature,
                            buttonCaps, &numCaps, HidDevice->Ppd)))
        {
            return bRet;
        }
    }

    numCaps = HidDevice->Caps.NumberFeatureValueCaps;

    if (numCaps > 0)
    {
        if (HIDP_STATUS_SUCCESS != HidP_GetValueCaps(HidP_Feature,
                           valueCaps, &numCaps, HidDevice->Ppd))
        {
            return bRet;
        }
    }

    numValues = 0;
    for (i = 0; i < HidDevice->Caps.NumberFeatureValueCaps; i++, valueCaps++)
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
    valueCaps = HidDevice->FeatureValueCaps;

    if (ULongAdd(HidDevice->Caps.NumberFeatureButtonCaps,
                                 numValues, &newFeatureDataLength) < 0)
    {
        return bRet;
    }

    HidDevice->FeatureDataLength = newFeatureDataLength;
    tmp = calloc(HidDevice->FeatureDataLength, sizeof(HID_DATA));
    HidDevice->FeatureData = data = (HID_DATA *)(tmp);

    if (NULL == data)
        return bRet;

    dataIdx = 0;
    for (i = 0;
         i < HidDevice->Caps.NumberFeatureButtonCaps;
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
                   HidP_Feature, buttonCaps->UsagePage, HidDevice->Ppd);

        data->ButtonData.Usages = new USAGE[data->ButtonData.MaxUsageLength];
        data->ReportID = buttonCaps -> ReportID;
    }

    for (i = 0; i < HidDevice->Caps.NumberFeatureValueCaps ; i++, valueCaps++)
    {
        if (valueCaps->IsRange)
        {
            for (usage = valueCaps->Range.UsageMin;
                 usage <= valueCaps->Range.UsageMax;
                 usage++)
            {
                if (dataIdx >= HidDevice->FeatureDataLength)
                    return bRet;

                data->IsButtonData = FALSE;
                data->Status = HIDP_STATUS_SUCCESS;
                data->UsagePage = valueCaps->UsagePage;
                data->ValueData.Usage = usage;
                data->ReportID = valueCaps -> ReportID;
                data++;
                dataIdx++;
            }
        }
        else
        {
            if (dataIdx >= HidDevice->FeatureDataLength)
                return bRet;

            data->IsButtonData = FALSE;
            data->Status = HIDP_STATUS_SUCCESS;
            data->UsagePage = valueCaps->UsagePage;
            data->ValueData.Usage = valueCaps->NotRange.Usage;
            data->ReportID = valueCaps -> ReportID;
            data++;
            dataIdx++;
        }
    }

    bRet = TRUE;
    return bRet;
}

BOOLEAN HidDevice::open(
    LPCSTR path, BOOL hasReadAccess, BOOL hasWriteAccess,
    BOOL isOverlapped, BOOL isExclusive)
{
    DWORD accessFlags = 0;
    DWORD sharingFlags = 0;
    BOOLEAN bRet = FALSE;
    RtlZeroMemory(&_dev, sizeof(HID_DEVICE));
    _handle = INVALID_HANDLE_VALUE;

    if (path == NULL)
        return FALSE;

    INT iDevicePathSize = INT(strlen(path) + 1);
    _dev.DevicePath = new char[iDevicePathSize];

    if (_dev.DevicePath == NULL)
        goto Done;

    StringCbCopyA(_dev.DevicePath, iDevicePathSize, path);

    if (hasReadAccess)
        accessFlags |= GENERIC_READ;

    if (hasWriteAccess)
        accessFlags |= GENERIC_WRITE;

    if (!isExclusive)
        sharingFlags = FILE_SHARE_READ | FILE_SHARE_WRITE;

    _handle = CreateFileA(path, accessFlags, sharingFlags, NULL, OPEN_EXISTING, 0, NULL);

    if (handle() == INVALID_HANDLE_VALUE)
        goto Done;

    _dev.OpenedForRead = hasReadAccess;
    _dev.OpenedForWrite = hasWriteAccess;
    _dev.OpenedOverlapped = isOverlapped;
    _dev.OpenedExclusive = isExclusive;

    if (!HidD_GetPreparsedData(handle(), &_dev.Ppd))
        goto Done;

    if (!HidD_GetAttributes(handle(), &_dev.Attributes))
        goto Done;

    if (!HidP_GetCaps(_dev.Ppd, &_dev.Caps))
        goto Done;

    if (_FillDeviceInfo() == FALSE)
        goto Done;

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
    if (HidDevice->DevicePath != NULL)
    {
        free(HidDevice->DevicePath);
        HidDevice->DevicePath = NULL;
    }

    if (handle() != INVALID_HANDLE_VALUE)
    {
        CloseHandle(_handle);
        _handle = INVALID_HANDLE_VALUE;
    }

    if (NULL != HidDevice->Ppd)
    {
        HidD_FreePreparsedData(HidDevice->Ppd);
        HidDevice -> Ppd = NULL;
    }

    if (NULL != HidDevice -> InputReportBuffer)
    {
        free(HidDevice -> InputReportBuffer);
        HidDevice -> InputReportBuffer = NULL;
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


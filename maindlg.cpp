#include "maindlg.h"
#include "hclient.h"
#include "resource.h"
#include "readdlg.h"
#include "writedlg.h"
#include "list.h"
#include <strsafe.h>
#include <dbt.h>

typedef DEVICE_LIST_NODE *PDEVICE_LIST_NODE;

MainDialog::MainDialog(HINSTANCE hInstance) : _hInstance(hInstance)
{
    _instance = this;
    _readDlg = new ReadDialog(hInstance);
    _writeDlg = new WriteDialog(hInstance);
}

MainDialog::~MainDialog()
{
    delete[] _readDlg;
}

void MainDialog::create()
{
    INT_PTR ret = DialogBox(_hInstance, TEXT("MAIN_DIALOG"), NULL, bMainDlgProc);

    if (ret < 0)
    {
        throw "Unable to create MainDialog";
    }
}

MainDialog *MainDialog::_instance;

INT_PTR CALLBACK
MainDialog::bMainDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    return _instance->dlgProc(hDlg, message, wParam, lParam);
}

HidDevice *MainDialog::_getCurrentDevice(HWND hDlg)
{
    INT i = INT(SendDlgItemMessage(hDlg, IDC_DEVICES, CB_GETCURSEL, 0, 0));

    if (i == CB_ERR)
        return nullptr;

    LRESULT lresult = SendDlgItemMessageA(hDlg, IDC_DEVICES, CB_GETITEMDATA, i, 0);
    HID_DEVICE *dev = reinterpret_cast<HID_DEVICE *>(lresult);
    HidDevice *ret = new HidDevice;
    ret->set(*dev);
    return ret;
}

BOOL MainDialog::_bGetData(rWriteDataStruct *pItems, INT iCount, HWND hParent)
{
    rGetWriteDataParams rParams;
    static rWriteDataStruct arTempItems[MAX_WRITE_ELEMENTS];

    if (iCount > MAX_WRITE_ELEMENTS)
        iCount = MAX_WRITE_ELEMENTS;

    memcpy( &(arTempItems[0]), pItems, sizeof(rWriteDataStruct)*iCount);
    rParams.iCount = iCount;
    rParams.prItems = &(arTempItems[0]);
    INT iResult = _writeDlg->create(hParent, &rParams);

    if (iResult)
    {
        memcpy(pItems, arTempItems, sizeof(rWriteDataStruct) * iCount);
    }
    return iResult;
}

BOOL MainDialog::_registerHidDevice(HWND WindowHandle, DEVICE_LIST_NODE *DeviceNode)
{
    DEV_BROADCAST_HANDLE broadcastHandle;
    broadcastHandle.dbch_size = sizeof(DEV_BROADCAST_HANDLE);
    broadcastHandle.dbch_devicetype = DBT_DEVTYP_HANDLE;
    broadcastHandle.dbch_handle = DeviceNode -> HidDeviceInfo.HidDevice;

    DeviceNode->NotificationHandle = RegisterDeviceNotification(
              WindowHandle, &broadcastHandle, DEVICE_NOTIFY_WINDOW_HANDLE);

    return DeviceNode->NotificationHandle != NULL;
}

VOID MainDialog::DestroyDeviceListCallback(LIST_ENTRY *ListNode)
{
    PDEVICE_LIST_NODE deviceNode = PDEVICE_LIST_NODE(ListNode);

    // The callback function needs to do the following steps...
    //   1) Close the HidDevice
    //   2) Unregister device notification (if registered)
    //   3) Free the allocated memory block
    CloseHidDevice(&deviceNode->HidDeviceInfo);

    if (deviceNode->NotificationHandle != NULL)
        UnregisterDeviceNotification(deviceNode->NotificationHandle);

    free(deviceNode);
}

void MainDialog::addString(HWND hctrl, LPCSTR str, ULONG value)
{
    CHAR szTempBuff[SMALL_BUFF];
    StringCbPrintfA(szTempBuff, SMALL_BUFF, str, value);
    SendMessageA(hctrl, LB_ADDSTRING, 0, LPARAM(szTempBuff));
}

void MainDialog::_displayDeviceCaps(HIDP_CAPS *pCaps, HWND hControl)
{
    SendMessageA(hControl, LB_RESETCONTENT, 0, 0);
    addString(hControl, "Usage Page: 0x%x", pCaps->UsagePage);
    addString(hControl, "Usage: 0x%x", pCaps->Usage);
    addString(hControl, "Input report byte length: %d", pCaps->InputReportByteLength);
    addString(hControl, "Output report byte length: %d", pCaps->OutputReportByteLength);
    addString(hControl, "Feature report byte length: %d", pCaps->FeatureReportByteLength);
    addString(hControl, "Number of collection nodes %d: ", pCaps->NumberLinkCollectionNodes);
}

void MainDialog::_displayButtonAttributes(HIDP_BUTTON_CAPS *pButton, HWND hControl)
{
    CHAR szTempBuff[SMALL_BUFF];
    StringCbPrintfA(szTempBuff, SMALL_BUFF, "Report ID: 0x%x", pButton->ReportID);
    SendMessageA(hControl, LB_ADDSTRING, 0, LPARAM(szTempBuff));
    StringCbPrintfA(szTempBuff, SMALL_BUFF, "Usage Page: 0x%x", pButton->UsagePage);
    SendMessageA(hControl, LB_ADDSTRING, 0, LPARAM(szTempBuff));
    StringCbPrintfA(szTempBuff, SMALL_BUFF, "Alias: %s", pButton->IsAlias ? "TRUE" : "FALSE");
    SendMessageA(hControl, LB_ADDSTRING, 0, LPARAM(szTempBuff));
    StringCbPrintfA(szTempBuff, SMALL_BUFF, "Link Collection: %hu", pButton->LinkCollection);
    SendMessageA(hControl, LB_ADDSTRING, 0, LPARAM(szTempBuff));
    StringCbPrintfA(szTempBuff, SMALL_BUFF, "Link Usage Page: 0x%x", pButton->LinkUsagePage);
    SendMessageA(hControl, LB_ADDSTRING, 0, LPARAM(szTempBuff));
    StringCbPrintfA(szTempBuff, SMALL_BUFF, "Link Usage: 0x%x", pButton->LinkUsage);
    SendMessageA(hControl, LB_ADDSTRING, 0, LPARAM(szTempBuff));

    if (pButton->IsRange)
    {
        StringCbPrintfA(szTempBuff, SMALL_BUFF, "Usage Min: 0x%x, Usage Max: 0x%x",
                       pButton->Range.UsageMin, pButton->Range.UsageMax);
    }
    else
    {
        StringCbPrintfA(szTempBuff, SMALL_BUFF, "Usage: 0x%x",pButton->NotRange.Usage);

    }
    SendMessageA(hControl, LB_ADDSTRING, 0, LPARAM(szTempBuff));

    if (pButton->IsRange)
    {
        StringCbPrintfA(szTempBuff, SMALL_BUFF,
                 "Data Index Min: 0x%x, Data Index Max: 0x%x",
                 pButton->Range.DataIndexMin, pButton->Range.DataIndexMax);
    }
    else
    {
         StringCbPrintfA(szTempBuff, SMALL_BUFF, "DataIndex: 0x%x", pButton->NotRange.DataIndex);
    }

    SendMessageA(hControl, LB_ADDSTRING, 0, LPARAM(szTempBuff));

    if (pButton->IsStringRange)
    {
        StringCbPrintfA(szTempBuff, SMALL_BUFF, "String Min: 0x%x, String Max: 0x%x",
                       pButton->Range.StringMin, pButton->Range.StringMax);
    }
    else
    {
        StringCbPrintfA(szTempBuff, SMALL_BUFF, "String Index: 0x%x", pButton->NotRange.StringIndex);
    }

    SendMessageA(hControl, LB_ADDSTRING, 0, LPARAM(szTempBuff));

    if (pButton->IsDesignatorRange)
    {
        StringCbPrintfA(szTempBuff, SMALL_BUFF,
                       "Designator Min: 0x%x, Designator Max: 0x%x",
                       pButton->Range.DesignatorMin,
                       pButton->Range.DesignatorMax);
    }
    else
    {
        StringCbPrintfA(szTempBuff, SMALL_BUFF, "Designator Index: 0x%x",
                       pButton->NotRange.DesignatorIndex);
    }
    SendMessageA(hControl, LB_ADDSTRING, 0, LPARAM(szTempBuff));

    if (pButton->IsAbsolute)
        SendMessageA(hControl, LB_ADDSTRING, 0, LPARAM("Absolute: Yes"));
    else
        SendMessageA(hControl, LB_ADDSTRING, 0, LPARAM("Absolute: No"));
}

void MainDialog::_loadDevices(HWND hDeviceCombo)
{
    CHAR szTempBuff[SMALL_BUFF];
    INT iIndex;
    SendMessage(hDeviceCombo, CB_RESETCONTENT, 0, 0);
    CListIterator it(&g_physDevList);

    for (INT i = 0; it.hasNext(); i++)
    {
        DEVICE_LIST_NODE *currNode = PDEVICE_LIST_NODE(it.next());

        if (currNode->HidDeviceInfo.HidDevice != INVALID_HANDLE_VALUE)
        {
            StringCbPrintfA(szTempBuff, SMALL_BUFF,
                    "Device #%d: VID: 0x%04X  PID: 0x%04X  UsagePage: 0x%X  Usage: 0x%X", i,
                    currNode->HidDeviceInfo.Attributes.VendorID,
                    currNode->HidDeviceInfo.Attributes.ProductID,
                    currNode->HidDeviceInfo.Caps.UsagePage,
                    currNode->HidDeviceInfo.Caps.Usage);
        }
        else
        {
            StringCbPrintfA(szTempBuff, SMALL_BUFF, "*Device #%d: %s", i,
                            (NULL != currNode->HidDeviceInfo.DevicePath) ? (currNode->HidDeviceInfo.DevicePath) : "");
        }

        iIndex = INT(SendMessageA(hDeviceCombo, CB_ADDSTRING, 0, LPARAM(szTempBuff)));

        if (CB_ERR != iIndex && INVALID_HANDLE_VALUE != currNode->HidDeviceInfo.HidDevice)
        {
            SendMessageA(hDeviceCombo, CB_SETITEMDATA, iIndex, (LPARAM)(&currNode->HidDeviceInfo));
        }
    }

    SendMessage(hDeviceCombo, CB_SETCURSEL, 0, 0);
}

void MainDialog::_displayFeatureValues(HidDevice *pDevice, HWND hControl)
{
    CHAR szTempBuff[SMALL_BUFF];
    SendMessage(hControl, LB_RESETCONTENT, 0, 0);
    HIDP_VALUE_CAPS *pValueCaps = pDevice->getp()->FeatureValueCaps;

    for (INT iLoop = 0; iLoop < pDevice->getp()->Caps.NumberFeatureValueCaps; iLoop++)
    {
        StringCbPrintfA(szTempBuff, SMALL_BUFF, "Feature value cap # %d", iLoop);
        INT iIndex = INT(SendMessageA(hControl, LB_ADDSTRING, 0, LPARAM(szTempBuff)));

        if (iIndex != -1)
            SendMessageA(hControl, LB_SETITEMDATA, iIndex, LPARAM(pValueCaps));

        pValueCaps++;
    }

    SendMessage(hControl, LB_SETCURSEL, 0, 0);
}

void MainDialog::_displayOutputValues(HidDevice *pDevice, HWND hControl)
{
    CHAR szTempBuff[SMALL_BUFF];
    SendMessage(hControl, LB_RESETCONTENT, 0, 0);
    HIDP_VALUE_CAPS *pValueCaps = pDevice->getp()->OutputValueCaps;

    for (INT iLoop = 0; iLoop < pDevice->getp()->Caps.NumberOutputValueCaps; iLoop++)
    {
        StringCbPrintfA(szTempBuff, SMALL_BUFF, "Output value cap # %d", iLoop);
        INT iIndex = INT(SendMessageA(hControl, LB_ADDSTRING, 0, LPARAM(szTempBuff)));

        if (iIndex != -1)
            SendMessageA(hControl, LB_SETITEMDATA, iIndex, LPARAM(pValueCaps));

        pValueCaps++;
    }

    SendMessage(hControl, LB_SETCURSEL, 0, 0);
}

void MainDialog::_displayValueAttributes(HIDP_VALUE_CAPS *pValue, HWND hControl)
{
    CHAR szTempBuff[SMALL_BUFF];
    StringCbPrintfA(szTempBuff, SMALL_BUFF, "Report ID 0x%x", pValue->ReportID);
    SendMessageA(hControl, LB_ADDSTRING, 0, LPARAM(szTempBuff));
    StringCbPrintfA(szTempBuff, SMALL_BUFF, "Usage Page: 0x%x", pValue->UsagePage);
    SendMessageA(hControl, LB_ADDSTRING, 0, LPARAM(szTempBuff));
    StringCbPrintfA(szTempBuff, SMALL_BUFF, "Bit size: 0x%x", pValue->BitSize);
    SendMessageA(hControl, LB_ADDSTRING, 0, (LPARAM) szTempBuff);
    StringCbPrintfA(szTempBuff, SMALL_BUFF, "Report Count: 0x%x", pValue->ReportCount);
    SendMessageA(hControl, LB_ADDSTRING, 0, (LPARAM) szTempBuff);
    StringCbPrintfA(szTempBuff, SMALL_BUFF, "Unit Exponent: 0x%x", pValue->UnitsExp);
    SendMessageA(hControl, LB_ADDSTRING, 0, (LPARAM) szTempBuff);
    StringCbPrintfA(szTempBuff, SMALL_BUFF, "Has Null: 0x%x", pValue->HasNull);
    SendMessageA(hControl, LB_ADDSTRING, 0, (LPARAM) szTempBuff);

    if (pValue->IsAlias)
        StringCbPrintfA(szTempBuff, SMALL_BUFF, "Alias");
    else
        StringCbPrintfA(szTempBuff, SMALL_BUFF, "=====");

    SendMessageA(hControl, LB_ADDSTRING, 0, LPARAM(szTempBuff));

    if (pValue->IsRange)
    {
        StringCbPrintfA(szTempBuff, SMALL_BUFF, "Usage Min: 0x%x, Usage Max 0x%x",
                       pValue->Range.UsageMin, pValue->Range.UsageMax);
    }
    else
    {
        StringCbPrintfA(szTempBuff, SMALL_BUFF, "Usage: 0x%x", pValue->NotRange.Usage);
    }

    SendMessageA(hControl, LB_ADDSTRING, 0, LPARAM(szTempBuff));

    if (pValue->IsRange)
    {
        StringCbPrintfA(szTempBuff, SMALL_BUFF,
                       "Data Index Min: 0x%x, Data Index Max: 0x%x",
                       pValue->Range.DataIndexMin,
                       pValue->Range.DataIndexMax);
    }
    else
    {
        StringCbPrintfA(szTempBuff, SMALL_BUFF, "DataIndex: 0x%x", pValue->NotRange.DataIndex);
    }

    SendMessageA(hControl, LB_ADDSTRING, 0, LPARAM(szTempBuff));

    StringCbPrintfA(szTempBuff, SMALL_BUFF, "Physical Minimum: %d, Physical Maximum: %d",
                   pValue->PhysicalMin, pValue->PhysicalMax);

    SendMessageA(hControl, LB_ADDSTRING, 0, LPARAM(szTempBuff));

    StringCbPrintfA(szTempBuff, SMALL_BUFF, "Logical Minimum: 0x%x, Logical Maximum: 0x%x",
                   pValue->LogicalMin, pValue->LogicalMax);

    SendMessageA(hControl, LB_ADDSTRING, 0, (LPARAM) szTempBuff);

    if (pValue->IsStringRange)
    {
       StringCbPrintfA(szTempBuff, SMALL_BUFF, "String  Min: 0x%x String Max 0x%x",
                       pValue->Range.StringMin, pValue->Range.StringMax);
    }
    else
    {
        StringCbPrintfA(szTempBuff, SMALL_BUFF, "String Index: 0x%x",
                        pValue->NotRange.StringIndex);
    }
    SendMessageA(hControl, LB_ADDSTRING, 0, LPARAM(szTempBuff));

    if (pValue->IsDesignatorRange)
    {
        StringCbPrintfA(szTempBuff, SMALL_BUFF, "Designator Minimum: 0x%x, Max: 0x%x",
                        pValue->Range.DesignatorMin, pValue->Range.DesignatorMax);
    }
    else
    {
        StringCbPrintfA(szTempBuff, SMALL_BUFF, "Designator Index: 0x%x",
                        pValue->NotRange.DesignatorIndex);
    }

    SendMessageA(hControl, LB_ADDSTRING, 0, LPARAM(szTempBuff));

    if (pValue->IsAbsolute)
        SendMessageA(hControl, LB_ADDSTRING, 0, LPARAM("Absolute: Yes"));
    else
        SendMessageA(hControl, LB_ADDSTRING, 0, LPARAM("Absolute: No"));
}

void MainDialog::_displayInputButtons(HidDevice *pDevice, HWND hControl)
{
    CHAR szTempBuff[SMALL_BUFF];
    SendMessageA(hControl, LB_RESETCONTENT, 0, LPARAM(0));
    HIDP_BUTTON_CAPS *pButtonCaps = pDevice->getp()->InputButtonCaps;
    const USHORT nCaps = pDevice->getp()->Caps.NumberInputButtonCaps;

    for (INT iLoop = 0; iLoop < nCaps; iLoop++)
    {
        StringCbPrintfA(szTempBuff, SMALL_BUFF, "Input button cap # %d", iLoop);
        INT iIndex = INT(SendMessageA(hControl, LB_ADDSTRING, 0, LPARAM(szTempBuff)));

        if (iIndex != -1)
            SendMessageA(hControl, LB_SETITEMDATA, iIndex, LPARAM(pButtonCaps));

        pButtonCaps++;
    }

    SendMessage(hControl, LB_SETCURSEL, 0, 0);
}

void MainDialog::_displayFeatureButtons(HidDevice *pDevice, HWND hControl)
{
    CHAR szTempBuff[SMALL_BUFF];
    SendMessage(hControl, LB_RESETCONTENT, 0, 0);
    HIDP_BUTTON_CAPS *pButtonCaps = pDevice->getp()->FeatureButtonCaps;

    USHORT nCaps = pDevice->getp()->Caps.NumberFeatureButtonCaps;
    for (USHORT iLoop = 0; iLoop < nCaps; iLoop++)
    {
        StringCbPrintfA(szTempBuff, SMALL_BUFF, "Feature button cap # %d", iLoop);
        INT iIndex = INT(SendMessageA(hControl, LB_ADDSTRING, 0, LPARAM(szTempBuff)));

        if (iIndex != -1)
            SendMessageA(hControl, LB_SETITEMDATA, iIndex, LPARAM(pButtonCaps));

        pButtonCaps++;
    }
    SendMessage(hControl, LB_SETCURSEL, 0, 0);
}

void MainDialog::_displayInputValues(HidDevice *pDevice, HWND hCtrl)
{
    CHAR szTempBuff[SMALL_BUFF];
    SendMessage(hCtrl, LB_RESETCONTENT, 0, 0);
    HIDP_VALUE_CAPS *pValueCaps = pDevice->getp()->InputValueCaps;

    for (INT iLoop = 0; iLoop < pDevice->getp()->Caps.NumberInputValueCaps; iLoop++)
    {
        StringCbPrintfA(szTempBuff, SMALL_BUFF, "Input value cap # %d", iLoop);
        INT iIndex = INT(SendMessageA(hCtrl, LB_ADDSTRING, 0, LPARAM(szTempBuff)));

        if (iIndex != -1)
            SendMessageA(hCtrl, LB_SETITEMDATA, iIndex, LPARAM(pValueCaps));

        pValueCaps++;
    }

    SendMessage(hCtrl, LB_SETCURSEL, 0, 0);
}

void MainDialog::_displayOutputButtons(HidDevice *pDevice, HWND hControl)
{
    CHAR szTempBuff[SMALL_BUFF];
    SendMessage(hControl, LB_RESETCONTENT, 0, LPARAM(0));
    HIDP_BUTTON_CAPS *pButtonCaps = pDevice->getp()->OutputButtonCaps;

    for (INT iLoop = 0; iLoop < pDevice->getp()->Caps.NumberOutputButtonCaps; iLoop++)
    {
        StringCbPrintfA(szTempBuff, SMALL_BUFF, "Output button cap # %d", iLoop);
        INT iIndex = INT(SendMessageA(hControl, LB_ADDSTRING, 0, LPARAM(szTempBuff)));

        if (iIndex != -1)
            SendMessageA(hControl, LB_SETITEMDATA, iIndex, LPARAM(pButtonCaps));

        pButtonCaps++;
    }

    SendMessage(hControl, LB_SETCURSEL, 0, 0);
}

void MainDialog::_loadItemTypes(HWND hItemTypes)
{
    INT iIndex = INT(SendMessageA(hItemTypes, CB_ADDSTRING, 0, LPARAM("INPUT BUTTON")));

    if (iIndex == -1)
        return;

    SendMessageA(hItemTypes, CB_SETITEMDATA, iIndex, INPUT_BUTTON);
    iIndex = INT(SendMessageA(hItemTypes, CB_ADDSTRING, 0, LPARAM("INPUT VALUE")));

    if (iIndex != -1)
        SendMessageA(hItemTypes, CB_SETITEMDATA, iIndex, INPUT_VALUE);

    iIndex = INT(SendMessageA(hItemTypes, CB_ADDSTRING, 0, LPARAM("OUTPUT BUTTON")));

    if (iIndex != -1)
        SendMessageA(hItemTypes, CB_SETITEMDATA, iIndex, OUTPUT_BUTTON);

    iIndex = INT(SendMessageA(hItemTypes, CB_ADDSTRING, 0, LPARAM("OUTPUT VALUE")));

    if (iIndex != -1)
        SendMessageA(hItemTypes, CB_SETITEMDATA, iIndex, OUTPUT_VALUE);

    iIndex = INT(SendMessageA(hItemTypes, CB_ADDSTRING, 0, LPARAM("FEATURE BUTTON")));

    if (iIndex != -1)
        SendMessageA(hItemTypes, CB_SETITEMDATA, iIndex, FEATURE_BUTTON);

    iIndex = INT(SendMessageA(hItemTypes, CB_ADDSTRING, 0, (LPARAM) "FEATURE VALUE"));

    if (iIndex != -1)
        SendMessageA(hItemTypes, CB_SETITEMDATA, iIndex, FEATURE_VALUE);

    iIndex = INT(SendMessageA(hItemTypes, CB_ADDSTRING, 0, LPARAM("HID CAPS")));
    if (iIndex != -1)
        SendMessageA(hItemTypes, CB_SETITEMDATA, iIndex, HID_CAPS);

    iIndex = INT(SendMessageA(hItemTypes, CB_ADDSTRING, 0, LPARAM("DEVICE ATTRIBUTES")));

    if (iIndex != -1)
        SendMessageA(hItemTypes, CB_SETITEMDATA, iIndex, DEVICE_ATTRIBUTES);

    SendMessage(hItemTypes, CB_SETCURSEL, 0, 0);
}

void MainDialog::_displayDeviceAttributes(HIDD_ATTRIBUTES *pAttrib, HWND hCtrl)
{
    addString(hCtrl, "Vendor ID: 0x%x", pAttrib->VendorID);
    addString(hCtrl, "Product ID: 0x%x", pAttrib->ProductID);
    addString(hCtrl, "Version Number  0x%x", pAttrib->VersionNumber);
}

BOOL MainDialog::_setButtonUsages(HID_DATA *pCap, LPSTR pszInputString)
{
    CHAR szTempString[SMALL_BUFF];
    CHAR pszDelimiter[] = " ";
    CHAR *pszContext = NULL;
    const BOOL bNoError = TRUE;

    if (FAILED(StringCbCopyA(szTempString, SMALL_BUFF, pszInputString)))
        return FALSE;

    char *pszToken = strtok_s(szTempString, pszDelimiter, &pszContext);
    PUSAGE pUsageWalk = pCap->ButtonData.Usages;
    memset(pUsageWalk, 0, pCap->ButtonData.MaxUsageLength * sizeof(USAGE));

    for (INT iLoop = 0;
         ULONG(iLoop) < pCap->ButtonData.MaxUsageLength && pszToken != NULL && bNoError;
         iLoop++)
    {
        *pUsageWalk = USAGE(atoi(pszToken));
        pszToken = strtok_s(NULL, pszDelimiter, &pszContext);
        pUsageWalk++;
    }

    return bNoError;
}

BOOL MainDialog::_parseData(HID_DATA *pData, rWriteDataStruct rWriteData[],
                            int iCount, int *piErrorLine)
{
    BOOL noError = TRUE;
    HID_DATA *pWalk = pData;

    for (INT iCap = 0; iCap < iCount && noError; iCap++)
    {
        // Check to see if our data is a value cap or not
        if (!pWalk->IsButtonData)
        {
            pWalk -> ValueData.Value = atol(rWriteData[iCap].szValue);
        }
        else if (!_setButtonUsages(pWalk, rWriteData[iCap].szValue))
        {
            *piErrorLine = iCap;
            noError = FALSE;
        }

        pWalk++;
    }
    return noError;
}

INT MainDialog::_prepareDataFields(HID_DATA *pData, ULONG ulDataLength,
    rWriteDataStruct rWriteData[], int iMaxElements)
{
    INT i;
    HID_DATA *pWalk = pData;

    for (i = 0; i < iMaxElements && ((unsigned) i < ulDataLength); i++)
    {
        if (!pWalk->IsButtonData)
        {
            StringCbPrintfA(rWriteData[i].szLabel, MAX_LABEL,
                           "ValueCap; ReportID: 0x%x, UsagePage=0x%x, Usage=0x%x",
                           pWalk->ReportID, pWalk->UsagePage, pWalk->ValueData.Usage);
        }
        else
        {
            StringCbPrintfA(rWriteData[i].szLabel, MAX_LABEL,
                           "Button; ReportID: 0x%x, UsagePage=0x%x, UsageMin: 0x%x, UsageMax: 0x%x",
                           pWalk->ReportID, pWalk->UsagePage, pWalk->ButtonData.UsageMin,
                           pWalk->ButtonData.UsageMax);
        }
        pWalk++;
    }
    return i;
}

#define WM_UNREGISTER_HANDLE    WM_USER+1

INT_PTR MainDialog::dlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    static rWriteDataStruct rWriteData[MAX_OUTPUT_ELEMENTS];
    HIDP_BUTTON_CAPS *pButtonCaps;
    HID_DEVICE writeDevice;
    HidDevice writeDevice2;
    ZeroMemory(&writeDevice, sizeof(writeDevice));

    switch (message)
    {
    case WM_INITDIALOG:
        init(hDlg);
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_READ:
        {
            HidDevice *pDevice = _getCurrentDevice(hDlg);

            if (pDevice != nullptr && _readDlg != nullptr)
                _readDlg->create(hDlg, pDevice);
        }
            break;
        case IDC_WRITE:
        {
            HidDevice *pDevice = _getCurrentDevice(hDlg);

            if (pDevice == nullptr)
                break;

            BOOL status = writeDevice2.open(pDevice->devicePath(), FALSE, TRUE, FALSE, FALSE);
            writeDevice = writeDevice2.get();

            if (!status)
            {
                MessageBoxA(hDlg, "Couldn't open device for write access",
                           HCLIENT_ERROR, MB_ICONEXCLAMATION);

                break;
            }

            INT iCount = _prepareDataFields(writeDevice.OutputData,
                                        writeDevice.OutputDataLength,
                                        rWriteData, MAX_OUTPUT_ELEMENTS);

            if (_bGetData(rWriteData, iCount, hDlg))
            {
                INT iErrorLine;

                if (_parseData(writeDevice.OutputData, rWriteData, iCount, &iErrorLine))
                {
                    HidWrite(&writeDevice);
                }
                else
                {
                    CHAR szTempBuff[SMALL_BUFF];

                    if (StringCbPrintfA(szTempBuff, SMALL_BUFF,
                                   "Unable to parse line %x of output data",
                                   iErrorLine) < 0)
                    {
                        MessageBoxA(hDlg, "StringCbPrintf failed", HCLIENT_ERROR, MB_ICONEXCLAMATION);
                    }
                    else
                    {
                        MessageBoxA(hDlg, szTempBuff, HCLIENT_ERROR, MB_ICONEXCLAMATION);
                    }
                }
            }
            CloseHidDevice(&writeDevice);
        }
            break;
        case IDC_EXTCALLS:
            MessageBox(hDlg, TEXT("Extcalls not implemented"), TEXT("Note"), 0);
            break;
        case IDC_FEATURES:
            MessageBox(hDlg, TEXT("Features not implemented"), TEXT("Note"), 0);
            break;
        case IDC_DEVICES:
            switch (HIWORD(wParam))
            {
            case CBN_SELCHANGE:
            {
                HidDevice *pDevice2 = _getCurrentDevice(hDlg);
                EnableWindow(GetDlgItem(hDlg, IDC_EXTCALLS), pDevice2 != nullptr);

                EnableWindow(GetDlgItem(hDlg, IDC_READ), pDevice2 != nullptr &&
                                pDevice2->getp()->Caps.InputReportByteLength);

                EnableWindow(GetDlgItem(hDlg, IDC_WRITE), pDevice2 != nullptr &&
                                pDevice2->getp()->Caps.OutputReportByteLength);

                EnableWindow(GetDlgItem(hDlg, IDC_FEATURES),
                                pDevice2 != NULL && pDevice2->getp()->Caps.FeatureReportByteLength);

                PostMessageA(hDlg, WM_COMMAND, IDC_TYPE + (CBN_SELCHANGE<<16),
                            LPARAM(GetDlgItem(hDlg, IDC_TYPE)));
            }
                break;
            }
            break;
        case IDC_TYPE:
            switch (HIWORD(wParam))
            {
            case CBN_SELCHANGE:
            {
                HidDevice *pDevice2 = _getCurrentDevice(hDlg);
                SendDlgItemMessage(hDlg, IDC_ITEMS, LB_RESETCONTENT, 0, 0);
                SendDlgItemMessage(hDlg, IDC_ATTRIBUTES, LB_RESETCONTENT, 0, 0);

                if (pDevice2 != nullptr)
                {
                    INT iIndex = INT(SendDlgItemMessage(hDlg, IDC_TYPE, CB_GETCURSEL, 0, 0));
                    INT iItemType = INT(SendDlgItemMessageA(hDlg, IDC_TYPE, CB_GETITEMDATA, iIndex, 0));

                    switch (iItemType)
                    {
                    case INPUT_BUTTON:
                        _displayInputButtons(pDevice2, GetDlgItem(hDlg, IDC_ITEMS));
                        break;
                    case INPUT_VALUE:
                        _displayInputValues(pDevice2, GetDlgItem(hDlg, IDC_ITEMS));
                        break;
                    case OUTPUT_BUTTON:
                        _displayOutputButtons(pDevice2, GetDlgItem(hDlg, IDC_ITEMS));
                        break;
                    case OUTPUT_VALUE:
                        _displayOutputValues(pDevice2, GetDlgItem(hDlg, IDC_ITEMS));
                        break;
                    case FEATURE_BUTTON:
                        _displayFeatureButtons(pDevice2, GetDlgItem(hDlg, IDC_ITEMS));
                        break;
                    case FEATURE_VALUE:
                        _displayFeatureValues(pDevice2, GetDlgItem(hDlg, IDC_ITEMS));
                        break;
                    }

                    PostMessageA(hDlg, WM_COMMAND, IDC_ITEMS + (LBN_SELCHANGE << 16), LPARAM(GetDlgItem(hDlg, IDC_ITEMS)));
                }
            }
                break;
            }
            break;
        case IDC_ITEMS:
            switch(HIWORD(wParam))
            {
            case LBN_SELCHANGE:
            {
                INT iItemType = 0;
                INT iIndex = INT(SendDlgItemMessageA(hDlg, IDC_TYPE, CB_GETCURSEL, 0, 0));

                if (iIndex != -1)
                {
                    iItemType = INT(SendDlgItemMessageA(hDlg, IDC_TYPE, CB_GETITEMDATA, iIndex, 0));
                }

                iIndex = INT(SendDlgItemMessageA(hDlg, IDC_ITEMS, LB_GETCURSEL, 0, 0));

                switch (iItemType)
                {
                case INPUT_BUTTON:
                case OUTPUT_BUTTON:
                case FEATURE_BUTTON:
                    pButtonCaps = NULL;

                    if (iIndex != -1)
                    {
                        LRESULT ret = SendDlgItemMessageA(hDlg, IDC_ITEMS, LB_GETITEMDATA, iIndex, 0);
                        pButtonCaps = PHIDP_BUTTON_CAPS(ret);
                    }

                    SendDlgItemMessageA(hDlg, IDC_ATTRIBUTES, LB_RESETCONTENT, 0, 0);

                    if (pButtonCaps != NULL)
                        _displayButtonAttributes(pButtonCaps, GetDlgItem(hDlg, IDC_ATTRIBUTES));

                    break;
                case INPUT_VALUE:
                case OUTPUT_VALUE:
                case FEATURE_VALUE:
                {
                    PHIDP_VALUE_CAPS pValueCaps = NULL;

                    if (iIndex != -1)
                    {
                        pValueCaps = PHIDP_VALUE_CAPS(SendDlgItemMessageA(
                                    hDlg, IDC_ITEMS, LB_GETITEMDATA, iIndex, 0));
                    }

                    SendDlgItemMessageA(hDlg, IDC_ATTRIBUTES, LB_RESETCONTENT, 0, 0);

                    if (pValueCaps != NULL)
                    {
                        _displayValueAttributes(pValueCaps, GetDlgItem(hDlg, IDC_ATTRIBUTES));
                    }
                }
                    break;
                case HID_CAPS:
                {
                    HidDevice *pDevice2 = _getCurrentDevice(hDlg);
                    HID_DEVICE *pDevice = pDevice2->getp();

                    if (NULL != pDevice)
                    {
                        _displayDeviceCaps(&(pDevice->Caps), GetDlgItem(hDlg, IDC_ATTRIBUTES));
                    }
                }
                    break;
                case DEVICE_ATTRIBUTES:
                    HidDevice *pDevice2 = _getCurrentDevice(hDlg);
                    HID_DEVICE *pDevice = pDevice2->getp();

                    if (pDevice != NULL)
                    {
                        SendDlgItemMessageA(hDlg, IDC_ATTRIBUTES, LB_RESETCONTENT, 0, 0);
                        _displayDeviceAttributes(&(pDevice->Attributes), GetDlgItem(hDlg, IDC_ATTRIBUTES));
                    }
                    break;
                }
            }
                    break;
                }
                break;
            case IDC_ABOUT:
                MessageBoxA(hDlg,
                           "Sample HID client Application.  Microsoft Corp \nCopyright (C) 1997",
                           "About HClient", MB_ICONINFORMATION);
                break;
            case IDOK:
            case IDCANCEL:
                g_physDevList.destroyWithCallback(DestroyDeviceListCallback);
                EndDialog(hDlg, 0);
                break;
            }
            break;
        case WM_DEVICECHANGE:
            switch (wParam)
            {
            case DBT_DEVICEARRIVAL:
            {
                PDEV_BROADCAST_HDR broadcastHdr = PDEV_BROADCAST_HDR(lParam);

                if (DBT_DEVTYP_DEVICEINTERFACE == broadcastHdr->dbch_devicetype)
                {
                    DEV_BROADCAST_DEVICEINTERFACE_A *pbroadcastInterface;
                    DEVICE_LIST_NODE *currNode, *nextNode;
                    BOOLEAN phyDevListChanged = FALSE;
                    pbroadcastInterface = PDEV_BROADCAST_DEVICEINTERFACE_A(lParam);
                    currNode = reinterpret_cast<DEVICE_LIST_NODE *>(g_physDevList.head());

                    while (currNode != PDEVICE_LIST_NODE(g_physDevList.get()))
                    {
                        // save the next node pointer first as we may remove
                        //  the current node.
                        nextNode = PDEVICE_LIST_NODE(PLIST_ENTRY(currNode)->Flink);

                        if (currNode->HidDeviceInfo.DevicePath != NULL &&
                            strcmp(currNode->HidDeviceInfo.DevicePath,
                                   pbroadcastInterface->dbcc_name) == 0)
                        {
                            if (currNode->DeviceOpened == FALSE)
                            {
                                // Remove the node from the list
                                g_physDevList.remove(PLIST_ENTRY(currNode));

                                // Let it free the memory of the device path string
                                CloseHidDevice(&currNode->HidDeviceInfo);

                                // Free the memory of the node structure
                                free(currNode);

                                phyDevListChanged = TRUE;
                            }
                            else
                            {
                                return TRUE;
                            }
                        }

                        // Move to the next node
                        currNode = nextNode;
                    }

                    // In this structure, we are given the name of the device
                    //    to open.  So all that needs to be done is open
                    //    a new hid device with the string
                    DEVICE_LIST_NODE *listNode;
                    listNode = PDEVICE_LIST_NODE(malloc(sizeof(DEVICE_LIST_NODE)));

                    if (NULL == listNode)
                    {
                        MessageBoxA(hDlg, "Error -- Couldn't allocate memory for new device list node",
                                    HCLIENT_ERROR, MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
                    }
                    else
                    {
                        BOOLEAN res;
                        res = listNode->hidDevice.open(pbroadcastInterface->dbcc_name,
                                                       FALSE, FALSE, FALSE, FALSE);

                        listNode->HidDeviceInfo = listNode->hidDevice.get();

                        if (!res)
                        {
                            // Save the device path so it can be still listed.
                            INT iDevicePathSize = INT(strlen(pbroadcastInterface->dbcc_name)) + 1;
                            listNode->HidDeviceInfo.DevicePath = PCHAR(malloc(iDevicePathSize));

                            if (NULL == listNode -> HidDeviceInfo.DevicePath)
                            {
                                MessageBoxA(hDlg,
                                   "Error -- Couldn't allocate memory for new device path",
                                   HCLIENT_ERROR, MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);

                                free(listNode);
                                listNode = NULL;
                            }
                            else
                            {
                                StringCbCopyA(listNode->HidDeviceInfo.DevicePath, iDevicePathSize, pbroadcastInterface->dbcc_name);
                            }
                        }

                        if (listNode != NULL)
                        {
                            listNode->DeviceOpened = (INVALID_HANDLE_VALUE == listNode->HidDeviceInfo.HidDevice) ? FALSE : TRUE;

                            if (listNode -> DeviceOpened)
                            {
                                if (!_registerHidDevice(hDlg, listNode))
                                {
                                    MessageBoxA(hDlg, "Error -- Couldn't register handle notification",
                                            HCLIENT_ERROR, MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
                                }
                            }

                            g_physDevList.insertTail(PLIST_ENTRY(listNode));
                            phyDevListChanged = TRUE;
                        }
                    }

                    if (phyDevListChanged)
                    {
                        _loadDevices(GetDlgItem(hDlg, IDC_DEVICES));
                        PostMessageA(hDlg, WM_COMMAND, IDC_DEVICES + (CBN_SELCHANGE << 16), LPARAM(GetDlgItem(hDlg, IDC_DEVICES)));
                    }
                }
            }
                break;
            case DBT_DEVICEQUERYREMOVE:
            {
                DEV_BROADCAST_HDR *broadcastHdr = PDEV_BROADCAST_HDR(lParam);

                if (broadcastHdr->dbch_devicetype != DBT_DEVTYP_HANDLE)
                    return TRUE;

                PDEV_BROADCAST_HANDLE broadcastHandle = PDEV_BROADCAST_HANDLE(lParam);

                // Get the handle to the notification registration
                HDEVNOTIFY notificationHandle = broadcastHandle->dbch_hdevnotify;

                // Search the physical device list for the handle that
                //  was removed...
                PDEVICE_LIST_NODE currNode;
                currNode = PDEVICE_LIST_NODE(g_physDevList.head());

                // Find out the device to close its handle
                while (currNode != PDEVICE_LIST_NODE(g_physDevList.get()))
                {
                    if (currNode->NotificationHandle == notificationHandle)
                    {
                        if (currNode->DeviceOpened)
                        {
                            CloseHidDevice(&(currNode -> HidDeviceInfo));
                            currNode->DeviceOpened = FALSE;
                        }

                        break;
                    }

                    currNode = PDEVICE_LIST_NODE(PLIST_ENTRY(currNode)->Flink);
                }
            }
                return TRUE;
            case DBT_DEVICEREMOVEPENDING:
            case DBT_DEVICEREMOVECOMPLETE:
            {
                PDEV_BROADCAST_HDR broadcastHdr = PDEV_BROADCAST_HDR(lParam);

                if (DBT_DEVTYP_HANDLE != broadcastHdr->dbch_devicetype)
                    return TRUE;

                PDEV_BROADCAST_HANDLE broadcastHandle;
                broadcastHandle = PDEV_BROADCAST_HANDLE(lParam);

                // Get the handle for device notification
                HDEVNOTIFY notificationHandle = broadcastHandle->dbch_hdevnotify;

                // Search the physical device list for the handle that
                //  was removed...
                DEVICE_LIST_NODE *currNode;
                currNode = reinterpret_cast<DEVICE_LIST_NODE *>(g_physDevList.head());

                while (currNode != PDEVICE_LIST_NODE(g_physDevList.get()))
                {
                    if (currNode->NotificationHandle == notificationHandle)
                    {
                        PostMessageA(hDlg, WM_UNREGISTER_HANDLE, 0, LPARAM(currNode->NotificationHandle));

                        if (currNode->DeviceOpened)
                            CloseHidDevice(&currNode->HidDeviceInfo);

                        g_physDevList.remove(PLIST_ENTRY(currNode));
                        free(currNode);
                        _loadDevices(GetDlgItem(hDlg, IDC_DEVICES));

                        PostMessageA(hDlg, WM_COMMAND, IDC_DEVICES + (CBN_SELCHANGE << 16),
                                   LPARAM(GetDlgItem(hDlg, IDC_DEVICES)));

                        break;
                    }

                    // The node has not been found yet. Move on to the next
                    currNode = PDEVICE_LIST_NODE(PLIST_ENTRY(currNode)->Flink);
                }

                return TRUE;
            }
                break;
            default:
                break;
        }
        break;
    case WM_UNREGISTER_HANDLE:
        UnregisterDeviceNotification(HDEVNOTIFY(lParam));
        break;
    }
    return FALSE;
}

BOOLEAN MainDialog::init(HWND hDlg)
{
    ULONG numberDevices;
    g_physDevList.init();
    HID_DEVICE *tempDeviceList = nullptr;

    if (!_FindKnownHidDevices(&tempDeviceList, &numberDevices))
    {
        EndDialog(hDlg, 0);
        return FALSE;
    }

    HID_DEVICE *pDevice = tempDeviceList;

    for (INT iIndex = 0; ULONG(iIndex) < numberDevices; iIndex++, pDevice++)
    {
        DEVICE_LIST_NODE *listNode = PDEVICE_LIST_NODE(malloc(sizeof(DEVICE_LIST_NODE)));

        if (listNode == nullptr)
        {
            g_physDevList.destroyWithCallback(DestroyDeviceListCallback);
            CloseHidDevices(pDevice, numberDevices - iIndex);
            free(tempDeviceList);
            EndDialog(hDlg, 0);
            return FALSE;
        }

        listNode->HidDeviceInfo = *pDevice;
        listNode->DeviceOpened = (INVALID_HANDLE_VALUE == pDevice->HidDevice) ? FALSE : TRUE;

        if (listNode->DeviceOpened)
        {
            // Register this device node with the PnP system so the dialog
            //  window can recieve notification if this device is unplugged.
            if (!_registerHidDevice(hDlg, listNode))
            {
                CHAR szTempBuff[SMALL_BUFF];

                StringCbPrintfA(szTempBuff, SMALL_BUFF,
                               "Failed registering device notifications for device #%d. Device changes won't be handled",
                               iIndex);

                MessageBoxA(hDlg, szTempBuff, HCLIENT_ERROR, MB_ICONEXCLAMATION);
            }
        }

        g_physDevList.insertTail(PLIST_ENTRY(listNode));
    }

    // Free the temporary device list...It is no longer needed
    free(tempDeviceList);

    // Register for notification from the HidDevice class.  Doing so
    //  allows the dialog box to receive device change notifications
    //  whenever a new HID device is added to the system
    DEV_BROADCAST_DEVICEINTERFACE broadcastInterface;
    broadcastInterface.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
    broadcastInterface.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    HidD_GetHidGuid(&broadcastInterface.dbcc_classguid);
    static HDEVNOTIFY diNotifyHandle = RegisterDeviceNotification(hDlg, &broadcastInterface, DEVICE_NOTIFY_WINDOW_HANDLE);

    if (diNotifyHandle == NULL)
    {
        MessageBoxA(hDlg,
                   "Failed registering HID device class notifications. New devices won't be handled.",
                    HCLIENT_ERROR, MB_ICONEXCLAMATION);
    }

    // Update the device list box...
    _loadDevices(GetDlgItem(hDlg, IDC_DEVICES));
    _loadItemTypes(GetDlgItem(hDlg, IDC_TYPE));

    // Post a message that the device changed so the appropriate
    //   data for the first device in the system can be displayed
    PostMessageA(hDlg, WM_COMMAND, IDC_DEVICES + (CBN_SELCHANGE<<16),
                LPARAM(GetDlgItem(hDlg, IDC_DEVICES)));

    return FALSE;
}

/*++
Routine Description:
   Do the required PnP things in order to find all the HID devices in
   the system at this time.
--*/
BOOLEAN MainDialog::_FindKnownHidDevices(HID_DEVICE **HidDevices, ULONG *NumberDevices)
{
    SP_DEVICE_INTERFACE_DATA deviceInfoData;
    ULONG i;
    BOOLEAN done = FALSE;

    GUID hidGuid;
    HidD_GetHidGuid(&hidGuid);
    *HidDevices = NULL;
    *NumberDevices = 0;

    // Open a handle to the plug and play dev node.
    HDEVINFO hardwareDeviceInfo = INVALID_HANDLE_VALUE;
    hardwareDeviceInfo = SetupDiGetClassDevs(&hidGuid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

    if (INVALID_HANDLE_VALUE == hardwareDeviceInfo)
        goto Done;

    // Take a wild guess to start
    *NumberDevices = 4;
    done = FALSE;
    deviceInfoData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    i = 0;
    while (!done)
    {
        *NumberDevices *= 2;

        if (*HidDevices)
        {
            void *tmp = realloc(*HidDevices, *NumberDevices * sizeof(HID_DEVICE));
            HID_DEVICE *newHidDevices = reinterpret_cast<HID_DEVICE *>(tmp);

            if (NULL == newHidDevices)
                free(*HidDevices);

            *HidDevices = newHidDevices;
        }
        else
        {
            void *tmp = calloc(*NumberDevices, sizeof(HID_DEVICE));
            *HidDevices = reinterpret_cast<HID_DEVICE *>(tmp);
        }

        if (NULL == *HidDevices)
            goto Done;

        HID_DEVICE *hidDeviceInst = *HidDevices + i;

        for (; i < *NumberDevices; i++, hidDeviceInst++)
        {
            // Initialize an empty HID_DEVICE
            RtlZeroMemory(hidDeviceInst, sizeof(HID_DEVICE));
            hidDeviceInst->HidDevice = INVALID_HANDLE_VALUE;

            if (SetupDiEnumDeviceInterfaces(hardwareDeviceInfo, 0,
                                    &hidGuid, i, &deviceInfoData))
            {
                ULONG requiredLength = 0;

                SetupDiGetDeviceInterfaceDetail(hardwareDeviceInfo,
                        &deviceInfoData, NULL, 0, &requiredLength, NULL);

                ULONG predictedLength = requiredLength;
                SP_DEVICE_INTERFACE_DETAIL_DATA_A *functionClassDeviceData;
                functionClassDeviceData = PSP_DEVICE_INTERFACE_DETAIL_DATA_A(malloc(predictedLength));

                if (functionClassDeviceData)
                {
                    functionClassDeviceData->cbSize = sizeof (SP_DEVICE_INTERFACE_DETAIL_DATA);
                    ZeroMemory(functionClassDeviceData->DevicePath, sizeof(functionClassDeviceData->DevicePath));
                }
                else
                {
                    goto Done;
                }

                // Retrieve the information from Plug and Play.
                if (SetupDiGetDeviceInterfaceDetailA(hardwareDeviceInfo, &deviceInfoData,
                           functionClassDeviceData, predictedLength, &requiredLength, NULL))
                {
                    HidDevice tmp;
                    BOOLEAN ret;
                    ret = tmp.open(functionClassDeviceData->DevicePath, FALSE, FALSE, FALSE, FALSE);
                    *hidDeviceInst = tmp.get();

                    if (!ret)
                    {
                        // Save the device path so it can be still listed.
                        INT iDevicePathSize = INT(strlen(functionClassDeviceData->DevicePath)) + 1;

                        hidDeviceInst->DevicePath = (PCHAR)malloc(iDevicePathSize);

                        if (hidDeviceInst->DevicePath != NULL)
                        {
                            StringCbCopyA(hidDeviceInst->DevicePath, iDevicePathSize, functionClassDeviceData->DevicePath);
                        }
                    }
                }

                free(functionClassDeviceData);
                functionClassDeviceData = NULL;
            }
            else
            {
                if (ERROR_NO_MORE_ITEMS == GetLastError())
                {
                    done = TRUE;
                    break;
                }
            }
        }
    }

    *NumberDevices = i;
Done:
    if (done == FALSE)
    {
        if (*HidDevices != NULL)
        {
            free(*HidDevices);
            *HidDevices = NULL;
        }
    }

    if (INVALID_HANDLE_VALUE != hardwareDeviceInfo)
    {
        SetupDiDestroyDeviceInfoList (hardwareDeviceInfo);
        hardwareDeviceInfo = INVALID_HANDLE_VALUE;
    }

    return done;
}


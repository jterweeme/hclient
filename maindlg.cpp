#include "maindlg.h"
#include "resource.h"
#include "readdlg.h"
#include "writedlg.h"
#include "hidfinder.h"
#include "hidinfo.h"
#include <dbt.h>
#include <iostream>

typedef DEVICE_LIST_NODE *PDEVICE_LIST_NODE;

MainDialog::MainDialog(HINSTANCE hInstance) : _hInstance(hInstance)
{
    _instance = this;
}

MainDialog::~MainDialog()
{
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

HidDevice *MainDialog::_getCurrentDevice() const
{
    INT i = INT(_cbDevices.sendMsgA(CB_GETCURSEL, 0, 0));

    if (i == CB_ERR)
        return nullptr;

    LRESULT lresult = _cbDevices.sendMsgA(CB_GETITEMDATA, i, 0);
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

BOOL MainDialog::_registerHidDevice(HWND WindowHandle, DEVICE_LIST_NODE *DeviceNode) const
{
    DEV_BROADCAST_HANDLE broadcastHandle;
    broadcastHandle.dbch_size = sizeof(DEV_BROADCAST_HANDLE);
    broadcastHandle.dbch_devicetype = DBT_DEVTYP_HANDLE;
    broadcastHandle.dbch_handle = DeviceNode->HidDeviceInfo.HidDevice;

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

    delete deviceNode;
}

void MainDialog::_loadDevices()
{
    CHAR szTempBuff[SMALL_BUFF];
    INT iIndex;
    _cbDevices.sendMsgA(CB_RESETCONTENT, 0, 0);
    CListIterator it(&g_physDevList);

    for (INT i = 0; it.hasNext(); i++)
    {
        DEVICE_LIST_NODE *currNode = PDEVICE_LIST_NODE(it.next());

        if (currNode->HidDeviceInfo.HidDevice != INVALID_HANDLE_VALUE)
        {
            USHORT vid = currNode->HidDeviceInfo.Attributes.VendorID;
            USHORT pid = currNode->HidDeviceInfo.Attributes.ProductID;
            USHORT usagePage = currNode->HidDeviceInfo.Caps.UsagePage;
            USHORT usage = currNode->HidDeviceInfo.Caps.Usage;

            StringCbPrintfA(szTempBuff, SMALL_BUFF,
                    "Device #%d: VID: 0x%04X  PID: 0x%04X  UsagePage: 0x%X  Usage: 0x%X",
                    i, vid, pid, usagePage, usage);
        }
        else
        {
            StringCbPrintfA(szTempBuff, SMALL_BUFF, "*Device #%d: %s", i,
                            currNode->HidDeviceInfo.DevicePath != NULL ?
                            currNode->HidDeviceInfo.DevicePath : "");
        }

        iIndex = INT(_cbDevices.sendMsgA(CB_ADDSTRING, 0, LPARAM(szTempBuff)));

        if (iIndex != CB_ERR && INVALID_HANDLE_VALUE != currNode->HidDeviceInfo.HidDevice)
        {
            _cbDevices.sendMsgA(CB_SETITEMDATA, iIndex, LPARAM(&currNode->HidDeviceInfo));
        }
    }

    _cbDevices.sendMsgA(CB_SETCURSEL, 0, 0);
}

void MainDialog::_loadItemTypes() const
{
    INT iIndex;
    iIndex = INT(_cbItemType.sendMsgA(CB_ADDSTRING, 0, LPARAM("INPUT BUTTON")));
    _cbItemType.sendMsgA(CB_SETITEMDATA, iIndex, INPUT_BUTTON);
    iIndex = INT(_cbItemType.sendMsgA(CB_ADDSTRING, 0, LPARAM("INPUT VALUE")));
    _cbItemType.sendMsgA(CB_SETITEMDATA, iIndex, INPUT_VALUE);
    iIndex = INT(_cbItemType.sendMsgA(CB_ADDSTRING, 0, LPARAM("OUTPUT BUTTON")));
    _cbItemType.sendMsgA(CB_SETITEMDATA, iIndex, OUTPUT_BUTTON);
    iIndex = INT(_cbItemType.sendMsgA(CB_ADDSTRING, 0, LPARAM("OUTPUT VALUE")));
    _cbItemType.sendMsgA(CB_SETITEMDATA, iIndex, OUTPUT_VALUE);
    iIndex = INT(_cbItemType.sendMsgA(CB_ADDSTRING, 0, LPARAM("FEATURE BUTTON")));
    _cbItemType.sendMsgA(CB_SETITEMDATA, iIndex, FEATURE_BUTTON);
    iIndex = INT(_cbItemType.sendMsgA(CB_ADDSTRING, 0, LPARAM("FEATURE VALUE")));
    _cbItemType.sendMsgA(CB_SETITEMDATA, iIndex, FEATURE_VALUE);
    iIndex = INT(_cbItemType.sendMsgA(CB_ADDSTRING, 0, LPARAM("HID CAPS")));
    _cbItemType.sendMsgA(CB_SETITEMDATA, iIndex, HID_CAPS);
    iIndex = INT(_cbItemType.sendMsgA(CB_ADDSTRING, 0, LPARAM("DEVICE ATTRIBUTES")));
    _cbItemType.sendMsgA(CB_SETITEMDATA, iIndex, DEVICE_ATTRIBUTES);
    _cbItemType.sendMsgA(CB_SETCURSEL, 0, 0);
}

BOOL MainDialog::_setButtonUsages(HID_DATA *pCap, LPSTR pszInputString)
{
    CHAR szTempString[SMALL_BUFF];
    CHAR pszDelimiter[] = " ";
    CHAR *pszContext = NULL;
    const BOOL bNoError = TRUE;

    if (StringCbCopyA(szTempString, SMALL_BUFF, pszInputString) < 0)
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
            pWalk->ValueData.Value = atol(rWriteData[iCap].szValue);
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
    rWriteDataStruct rWriteData[], int iMaxElements) const
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

INT_PTR MainDialog::_command(HWND hDlg, WPARAM wParam)
{
    switch (LOWORD(wParam))
    {
    case IDC_READ:
        return _read(hDlg);
    case IDC_WRITE:
        return _write(hDlg);
    case IDC_EXTCALLS:
        MessageBox(hDlg, TEXT("Extcalls not implemented"), TEXT("Note"), 0);
        return TRUE;
    case IDC_FEATURES:
        MessageBox(hDlg, TEXT("Features not implemented"), TEXT("Note"), 0);
        return TRUE;
    case IDC_DEVICES:
        return _devices(hDlg, wParam);
    case IDC_TYPE:
        return _typeProc(hDlg, wParam);
    case IDC_ITEMS:
        return _itemsProc(hDlg, wParam);
    case IDC_ABOUT:
        MessageBoxA(hDlg, "Sample HID client Application.  Microsoft Corp \nCopyright (C) 1997",
                    "About HClient", MB_ICONINFORMATION);
        return FALSE;
    case IDOK:
    case IDCANCEL:
        g_physDevList.destroyWithCallback(DestroyDeviceListCallback);
        EndDialog(hDlg, 0);
        return FALSE;
    }
    return FALSE;
}

INT_PTR MainDialog::_write(HWND hDlg)
{
    HidDevice *pDevice = _getCurrentDevice();

    if (pDevice == nullptr)
        return FALSE;

    HidDevice writeDevice2;
    BOOL status = writeDevice2.open(pDevice->devicePath(), FALSE, TRUE, FALSE, FALSE);

    if (!status)
    {
        MessageBoxA(hDlg, "Couldn't open device for write access",
                   HCLIENT_ERROR, MB_ICONEXCLAMATION);

        return FALSE;
    }

    INT iCount = _prepareDataFields(writeDevice2.getp()->OutputData,
                                writeDevice2.getp()->OutputDataLength,
                                rWriteData, MAX_OUTPUT_ELEMENTS);

    if (_bGetData(rWriteData, iCount, hDlg))
    {
        INT iErrorLine;

        if (_parseData(writeDevice2.getp()->OutputData, rWriteData, iCount, &iErrorLine))
        {
            writeDevice2.write();
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
    writeDevice2.close();
    return FALSE;
}

INT_PTR MainDialog::_read(HWND hDlg)
{
    HidDevice *pDevice = _getCurrentDevice();

    if (pDevice != nullptr && _readDlg != nullptr)
        _readDlg->create(hDlg, pDevice);

    return FALSE;
}

INT_PTR MainDialog::_devices(HWND hDlg, WPARAM wParam)
{
    if (HIWORD(wParam != CBN_SELCHANGE))
        return FALSE;

    HidDevice *pDevice2 = _getCurrentDevice();
    EnableWindow(GetDlgItem(hDlg, IDC_EXTCALLS), pDevice2 != nullptr);

    EnableWindow(GetDlgItem(hDlg, IDC_READ), pDevice2 != nullptr &&
                    pDevice2->getp()->Caps.InputReportByteLength);

    EnableWindow(GetDlgItem(hDlg, IDC_WRITE), pDevice2 != nullptr &&
                    pDevice2->getp()->Caps.OutputReportByteLength);

    EnableWindow(GetDlgItem(hDlg, IDC_FEATURES),
                    pDevice2 != NULL && pDevice2->getp()->Caps.FeatureReportByteLength);

    PostMessageA(hDlg, WM_COMMAND, IDC_TYPE + (CBN_SELCHANGE<<16),
                LPARAM(GetDlgItem(hDlg, IDC_TYPE)));

    return FALSE;
}

INT_PTR MainDialog::_typeProc(HWND hDlg, WPARAM wParam)
{
    if (HIWORD(wParam) != CBN_SELCHANGE)
        return FALSE;

    HidDevice *pDevice2 = _getCurrentDevice();
    SendDlgItemMessage(hDlg, IDC_ITEMS, LB_RESETCONTENT, 0, 0);
    SendDlgItemMessage(hDlg, IDC_ATTRIBUTES, LB_RESETCONTENT, 0, 0);

    if (pDevice2 == nullptr)
        return FALSE;

    INT iIndex = INT(SendDlgItemMessage(hDlg, IDC_TYPE, CB_GETCURSEL, 0, 0));
    INT iItemType = INT(SendDlgItemMessageA(hDlg, IDC_TYPE, CB_GETITEMDATA, iIndex, 0));
    HidInfo h;

    switch (iItemType)
    {
    case INPUT_BUTTON:
        h.displayInputButtons(pDevice2, _lbItems);
        break;
    case INPUT_VALUE:
        h.displayInputValues(pDevice2, _lbItems);
        break;
    case OUTPUT_BUTTON:
        h.displayOutputButtons(pDevice2, _lbItems);
        break;
    case OUTPUT_VALUE:
        h.displayOutputValues(pDevice2, _lbItems);
        break;
    case FEATURE_BUTTON:
        h.displayFeatureButtons(pDevice2, _lbItems);
        break;
    case FEATURE_VALUE:
        h.displayFeatureValues(pDevice2, _lbItems);
        break;
    }

    PostMessageA(hDlg, WM_COMMAND, IDC_ITEMS + (LBN_SELCHANGE << 16), LPARAM(GetDlgItem(hDlg, IDC_ITEMS)));

    return FALSE;
}

INT_PTR MainDialog::dlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        return _init(hDlg);
    case WM_COMMAND:
        return _command(hDlg, wParam);
    case WM_DEVICECHANGE:
        switch (wParam)
        {
            case DBT_DEVICEARRIVAL:
                return _devArrival(hDlg, lParam);
            case DBT_DEVICEQUERYREMOVE:
                return _devQuery(lParam);
            case DBT_DEVICEREMOVEPENDING:
            case DBT_DEVICEREMOVECOMPLETE:
                return _devRemove(hDlg, lParam);
            default:
                break;
        }
        return FALSE;
    case WM_UNREGISTER_HANDLE:
        UnregisterDeviceNotification(HDEVNOTIFY(lParam));
        return FALSE;
    case WM_DESTROY:
        delete _writeDlg;
        delete _readDlg;
        return FALSE;
    }
    return FALSE;
}

INT_PTR MainDialog::_itemsProc(HWND hDlg, WPARAM wParam)
{
    if (HIWORD(wParam) != LBN_SELCHANGE)
        return FALSE;

    INT iItemType = 0;
    INT iIndex = INT(_cbItemType.sendMsgA(CB_GETCURSEL, 0, 0));

    if (iIndex != -1)
    {
        iItemType = INT(_cbItemType.sendMsgA(CB_GETITEMDATA, iIndex, 0));
    }

    iIndex = INT(_lbItems.sendMsgA(LB_GETCURSEL, 0, 0));

    switch (iItemType)
    {
    case INPUT_BUTTON:
    case OUTPUT_BUTTON:
    case FEATURE_BUTTON:
    {
        HIDP_BUTTON_CAPS *pButtonCaps = nullptr;

        if (iIndex != -1)
        {
            LRESULT ret = _lbItems.sendMsgA(LB_GETITEMDATA, iIndex, 0);
            pButtonCaps = PHIDP_BUTTON_CAPS(ret);
        }

        _lbAttr.sendMsgA(LB_RESETCONTENT, 0, 0);

        if (pButtonCaps != nullptr)
            HidInfo().displayButtonAttributes(pButtonCaps, GetDlgItem(hDlg, IDC_ATTRIBUTES));
    }
        break;
    case INPUT_VALUE:
    case OUTPUT_VALUE:
    case FEATURE_VALUE:
    {
        PHIDP_VALUE_CAPS pValueCaps = NULL;

        if (iIndex != -1)
        {
            pValueCaps = PHIDP_VALUE_CAPS(_lbItems.sendMsgA(LB_GETITEMDATA, iIndex, 0));
        }

        _lbAttr.sendMsgA(LB_RESETCONTENT, 0, 0);

        if (pValueCaps != NULL)
            HidInfo().displayValueAttributes(pValueCaps, _lbAttr);
    }
        return TRUE;
    case HID_CAPS:
        return _caps();
    case DEVICE_ATTRIBUTES:
        return _devAttr();
    }
    return TRUE;
}

INT_PTR MainDialog::_caps()
{
    HidDevice *pDevice2 = _getCurrentDevice();
    HID_DEVICE *pDevice = pDevice2->getp();

    if (pDevice != nullptr)
    {
        HidInfo().displayDeviceCaps(&pDevice->Caps, _lbAttr);
    }

    return TRUE;
}

INT_PTR MainDialog::_devAttr()
{
    HidDevice *pDevice2 = _getCurrentDevice();
    HID_DEVICE *pDevice = pDevice2->getp();

    if (pDevice != NULL)
    {
        _lbAttr.sendMsgA(LB_RESETCONTENT, 0, 0);
        HidInfo().displayDeviceAttributes(&pDevice->Attributes, _lbAttr);
    }

    return TRUE;
}

INT_PTR MainDialog::_devArrival(HWND hDlg, LPARAM lParam)
{
    PDEV_BROADCAST_HDR broadcastHdr = PDEV_BROADCAST_HDR(lParam);

    if (DBT_DEVTYP_DEVICEINTERFACE != broadcastHdr->dbch_devicetype)
        return TRUE;

    DEV_BROADCAST_DEVICEINTERFACE_A *pbroadcastInterface;
    DEVICE_LIST_NODE *currNode;
    BOOLEAN phyDevListChanged = FALSE;
    pbroadcastInterface = PDEV_BROADCAST_DEVICEINTERFACE_A(lParam);
#if 0
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
                delete currNode;
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
#endif

    CListIterator it(&g_physDevList);

    while (it.hasNext())
    {
        currNode = PDEVICE_LIST_NODE(it.next());

        if (currNode->HidDeviceInfo.DevicePath != NULL &&
                strcmp(currNode->HidDeviceInfo.DevicePath,
                       pbroadcastInterface->dbcc_name) == 0)
        {
            //g_physDevList.remove
        }
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
        res = listNode->hidDevice->open(pbroadcastInterface->dbcc_name,
                                       FALSE, FALSE, FALSE, FALSE);

        listNode->HidDeviceInfo = listNode->hidDevice->get();

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

                delete listNode;
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

            if (listNode->DeviceOpened)
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
        _loadDevices();
        PostMessageA(hDlg, WM_COMMAND, IDC_DEVICES + (CBN_SELCHANGE << 16), LPARAM(GetDlgItem(hDlg, IDC_DEVICES)));
    }

    return TRUE;
}

INT_PTR MainDialog::_devQuery(LPARAM lParam)
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
                CloseHidDevice(&currNode->HidDeviceInfo);
                currNode->DeviceOpened = FALSE;
            }

            break;
        }

        currNode = PDEVICE_LIST_NODE(PLIST_ENTRY(currNode)->Flink);
    }

    return FALSE;
}

INT_PTR MainDialog::_init(HWND hDlg)
{
    _cbDevices.import(hDlg, IDC_DEVICES);
    _cbItemType.import(hDlg, IDC_TYPE);
    _lbItems.import(hDlg, IDC_ITEMS);
    _lbAttr.import(hDlg, IDC_ATTRIBUTES);
    _readDlg = new ReadDialog(_hInstance);
    _writeDlg = new WriteDialog(_hInstance);
    g_physDevList.init();
    HidFinder hidFinder;

    INT iIndex = 0;
    while (hidFinder.hasNext())
    {
        HidDevice *dev = hidFinder.next();
        DEVICE_LIST_NODE *listNode = new DEVICE_LIST_NODE;
        listNode->hidDevice = dev;
        listNode->HidDeviceInfo = dev->get();
        listNode->DeviceOpened = dev->handle() ? FALSE : TRUE;

        if (listNode->DeviceOpened)
        {
            if (!_registerHidDevice(hDlg, listNode))
            {
                CHAR szTempBuff[SMALL_BUFF];

                StringCbPrintfA(szTempBuff, SMALL_BUFF,
                               "Failed registering device notifications for device #%d. Device changes won't be handled",
                               iIndex);

                MessageBoxA(hDlg, szTempBuff, HCLIENT_ERROR, MB_ICONEXCLAMATION);
            }
        }

        if (dev->devicePath())
        {
            std::cout << dev->devicePath() << "\n";
            std::cout.flush();
        }

        g_physDevList.insertTail(PLIST_ENTRY(listNode));
        iIndex++;
    }

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
    _loadDevices();
    _loadItemTypes();

    // Post a message that the device changed so the appropriate
    //   data for the first device in the system can be displayed
    PostMessageA(hDlg, WM_COMMAND, IDC_DEVICES + (CBN_SELCHANGE<<16),
                LPARAM(GetDlgItem(hDlg, IDC_DEVICES)));

    return FALSE;
}

INT_PTR MainDialog::_devRemove(HWND hDlg, LPARAM lParam)
{
    DEV_BROADCAST_HDR *broadcastHdr = PDEV_BROADCAST_HDR(lParam);

    if (broadcastHdr->dbch_devicetype != DBT_DEVTYP_HANDLE)
        return TRUE;

    DEV_BROADCAST_HANDLE *broadcastHandle;
    broadcastHandle = PDEV_BROADCAST_HANDLE(lParam);
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
            delete currNode;
            _loadDevices();

            PostMessageA(hDlg, WM_COMMAND, IDC_DEVICES + (CBN_SELCHANGE << 16),
                       LPARAM(GetDlgItem(hDlg, IDC_DEVICES)));

            break;
        }

        // The node has not been found yet. Move on to the next
        currNode = PDEVICE_LIST_NODE(PLIST_ENTRY(currNode)->Flink);
    }

    return FALSE;
}


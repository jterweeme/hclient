#include "maindlg.h"
#include "resource.h"
#include "readdlg.h"
#include "writedlg.h"
#include "hidfinder.h"
#include "hidinfo.h"
#include "toolbox.h"
#include <iostream>

#define MAX_WRITE_ELEMENTS 100

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
        throw "Unable to create MainDialog";
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
    HidDevice *ret = reinterpret_cast<HidDevice *>(lresult);
    return ret;
}

INT MainDialog::_bGetData()
{
    INT_PTR ret = _writeDlg->create2(_hDlg, &_writeData);

    return INT(ret);
}

void MainDialog::_loadDevices()
{
    CHAR szTempBuff[SMALL_BUFF];
    _cbDevices.reset();
    std::vector<HidDevice *>::iterator it = _devList.begin();
    INT i = 0;

    while (it != _devList.end())
    {
        HidDevice *dev = *it;

        if (dev->handle() != INVALID_HANDLE_VALUE)
        {
            USHORT vid = dev->attributes()->VendorID;
            USHORT pid = dev->attributes()->ProductID;
            const HIDP_CAPS *caps = dev->caps();
            USHORT usagePage = caps->UsagePage;
            USHORT usage = caps->Usage;

            StringCbPrintfA(szTempBuff, SMALL_BUFF,
                    "Device #%d: VID: 0x%04X  PID: 0x%04X  UsagePage: 0x%X  Usage: 0x%X",
                    i, vid, pid, usagePage, usage);
        }
        else
        {
            StringCbPrintfA(szTempBuff, SMALL_BUFF, "*Device #%d: %s", i, dev->devicePath());
        }

        INT iIndex = INT(_cbDevices.addStr(szTempBuff));

        if (iIndex != CB_ERR && dev->handle() != INVALID_HANDLE_VALUE)
        {
            //save pointer in combobox
            _cbDevices.setItemDataA(iIndex, LPARAM(dev));
        }

        it++, i++;
    }

    _cbDevices.sendMsgA(CB_SETCURSEL, 0, 0);
}

void MainDialog::_loadItemTypes()
{
    INT iIndex;
    iIndex = INT(_cbItemType.addStr("INPUT BUTTON"));
    _cbItemType.setItemDataA(iIndex, INPUT_BUTTON);
    iIndex = INT(_cbItemType.addStr("INPUT VALUE"));
    _cbItemType.setItemDataA(iIndex, INPUT_VALUE);
    iIndex = INT(_cbItemType.addStr("OUTPUT BUTTON"));
    _cbItemType.setItemDataA(iIndex, OUTPUT_BUTTON);
    iIndex = INT(_cbItemType.addStr("OUTPUT VALUE"));
    _cbItemType.setItemDataA(iIndex, OUTPUT_VALUE);
    iIndex = INT(_cbItemType.addStr("FEATURE BUTTON"));
    _cbItemType.setItemDataA(iIndex, FEATURE_BUTTON);
    iIndex = INT(_cbItemType.addStr("FEATURE VALUE"));
    _cbItemType.setItemDataA(iIndex, FEATURE_VALUE);
    iIndex = INT(_cbItemType.addStr("HID CAPS"));
    _cbItemType.setItemDataA(iIndex, HID_CAPS);
    iIndex = INT(_cbItemType.addStr("DEVICE ATTRIBUTES"));
    _cbItemType.setItemDataA(iIndex, DEVICE_ATTRIBUTES);
    _cbItemType.sendMsgA(CB_SETCURSEL, 0, 0);
}

void MainDialog::_setButtonUsages(HidData *pCap, LPCSTR pszInputString)
{
    CHAR szTempString[SMALL_BUFF];
    CHAR pszDelimiter[] = " ";
    CHAR *pszContext = NULL;

    if (StringCbCopyA(szTempString, SMALL_BUFF, pszInputString) < 0)
        throw "String copy";

    char *pszToken = strtok_s(szTempString, pszDelimiter, &pszContext);
    PUSAGE pUsageWalk = pCap->ButtonData.Usages;
    memset(pUsageWalk, 0, pCap->ButtonData.MaxUsageLength * sizeof(USAGE));

    for (INT iLoop = 0;
         ULONG(iLoop) < pCap->ButtonData.MaxUsageLength && pszToken != NULL;
         iLoop++)
    {
        *pUsageWalk = USAGE(atoi(pszToken));
        pszToken = strtok_s(NULL, pszDelimiter, &pszContext);
        pUsageWalk++;
    }
}

BOOL MainDialog::_parseData(HidData *pData, int iCount)
{
    for (INT i = 0; i < iCount; i++, pData++)
    {
        if (!pData->IsButtonData)
            pData->ValueData.Value = atol(_writeData.at(i).getValue().c_str());
        else
            _setButtonUsages(pData, _writeData.at(i).getValue().c_str());
    }
    return TRUE;
}

ULONG MainDialog::_prepareDataFields2(HidData *pData, ULONG len)
{
    HidData *pWalk = pData;

    ULONG i = 0;
    while (i < MAX_OUTPUT_ELEMENTS && i < len)
    {
        char buf[300];

        if (!pWalk->IsButtonData)
        {
            StringCbPrintfA(buf, 300,
                           "ValueCap; ReportID: 0x%x, UsagePage=0x%x, Usage=0x%x",
                           pWalk->ReportID, pWalk->UsagePage, pWalk->ValueData.Usage);
        }
        else
        {
            StringCbPrintfA(buf, 300,
                     "Button; ReportID: 0x%x, "
                     "UsagePage=0x%x, "
                     "UsageMin: 0x%x, UsageMax: 0x%x",
                     pWalk->ReportID, pWalk->UsagePage,
                     pWalk->ButtonData.UsageMin,
                     pWalk->ButtonData.UsageMax);
        }

        WriteData w;
        w.setLabel(buf);
        _writeData.push_back(w);
        pWalk++, i++;
    }
    return i;
}

INT_PTR MainDialog::_command(HWND hDlg, WPARAM wParam)
{
    switch (LOWORD(wParam))
    {
    case IDC_READ:
        return _read();
    case IDC_WRITE:
        return _write();
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
        return _itemsProc(wParam);
    case IDC_ABOUT:
        MessageBoxA(hDlg, "Sample HID client Application.  "
                    "Microsoft Corp \nCopyright (C) 1997",
                    "About HClient", MB_ICONINFORMATION);
        return FALSE;
    case IDOK:
    case IDCANCEL:
        EndDialog(hDlg, 0);
        return FALSE;
    }
    return FALSE;
}

INT_PTR MainDialog::_write()
{
    HidDevice *pDevice = _getCurrentDevice();

    if (pDevice == nullptr)
        return FALSE;

    HidDevice writeDev;

    try
    {
        writeDev.open(pDevice->devicePath(), FALSE, TRUE, FALSE, FALSE);
    }
    catch (...)
    {
        Toolbox().errorbox(_hDlg, "Couldn't open device for write access");
        return FALSE;
    }

    INT iCount = _prepareDataFields2(writeDev.output()->_data,
                                     writeDev.output()->dataLength());

    if (_bGetData())
    {
        _parseData(writeDev.output()->_data, iCount);
        writeDev.write();
    }
    writeDev.close();
    return FALSE;
}

INT_PTR MainDialog::_read()
{
    HidDevice *pDevice = _getCurrentDevice();

    if (pDevice == nullptr)
        throw "Device is null pointer!";

    _readDlg->create(_hDlg, pDevice);
    return FALSE;
}

/*
 * Wanneer op de devices combobox wordt geklikt
 * en er een device wordt geselecteerd
*/
INT_PTR MainDialog::_devices(HWND hDlg, WPARAM wParam)
{
    if (HIWORD(wParam != CBN_SELCHANGE))
        return FALSE;

    HidDevice *pDevice2 = _getCurrentDevice();

    if (pDevice2 == nullptr)
        throw "Device null pointer!";

    BOOL input = pDevice2->caps()->InputReportByteLength > 0 ? TRUE : FALSE;
    BOOL output = pDevice2->caps()->OutputReportByteLength > 0 ? TRUE : FALSE;
    BOOL feature = pDevice2->caps()->FeatureReportByteLength > 0 ? TRUE : FALSE;
    EnableWindow(GetDlgItem(hDlg, IDC_EXTCALLS), pDevice2 != nullptr);
    EnableWindow(GetDlgItem(hDlg, IDC_READ), pDevice2 != nullptr && input);
    EnableWindow(GetDlgItem(hDlg, IDC_WRITE), pDevice2 != nullptr && output);
    EnableWindow(GetDlgItem(hDlg, IDC_FEATURES), pDevice2 != nullptr && feature);

    PostMessageA(hDlg, WM_COMMAND, IDC_TYPE + (CBN_SELCHANGE<<16),
                 LPARAM(GetDlgItem(hDlg, IDC_TYPE)));

    return FALSE;
}

INT_PTR MainDialog::_typeProc(HWND hDlg, WPARAM wParam)
{
    if (HIWORD(wParam) != CBN_SELCHANGE)
        return FALSE;

    HidDevice *pDevice2 = _getCurrentDevice();
    _lbItems.reset();
    _lbAttr.reset();

    if (pDevice2 == nullptr)
        return FALSE;

    INT iIndex = INT(_cbItemType.sendMsgA(CB_GETCURSEL, 0, 0));
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

    PostMessageA(hDlg, WM_COMMAND, IDC_ITEMS + (LBN_SELCHANGE << 16),
                 LPARAM(GetDlgItem(hDlg, IDC_ITEMS)));

    return FALSE;
}

INT_PTR MainDialog::dlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM)
{
    switch (message)
    {
    case WM_INITDIALOG:
        return _init(hDlg);
    case WM_COMMAND:
        return _command(hDlg, wParam);
    case WM_DESTROY:
        delete _writeDlg;
        delete _readDlg;

        for (std::vector<HidDevice *>::iterator it = _devList.begin();
             it != _devList.end(); it++)
        {
            delete *it;
        }

        return FALSE;
    }
    return FALSE;
}

INT_PTR MainDialog::_itemsProc(WPARAM wParam)
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

        _lbAttr.reset();

        if (pButtonCaps != nullptr)
        {
            HidInfo().displayButtonAttributes(pButtonCaps, _lbAttr);
        }
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

        _lbAttr.reset();

        if (pValueCaps != NULL)
            HidInfo().displayValueAttributes(pValueCaps, _lbAttr);
    }
        return TRUE;
    case HID_CAPS:
    {
        HidDevice *pDevice2 = _getCurrentDevice();

        if (pDevice2 != nullptr)
            HidInfo().displayDeviceCaps(pDevice2->caps(), _lbAttr);
    }
        return TRUE;
    case DEVICE_ATTRIBUTES:
    {
        HidDevice *pDevice2 = _getCurrentDevice();

        if (pDevice2 != nullptr)
        {
            _lbAttr.reset();
            HidInfo().displayDeviceAttributes(pDevice2->attributes(), _lbAttr);
        }
    }
        return TRUE;
    }
    return TRUE;
}

INT_PTR MainDialog::_init(HWND hDlg)
{
    _hDlg = hDlg;
    _cbDevices.importFromDlg(hDlg, IDC_DEVICES);
    _cbItemType.importFromDlg(hDlg, IDC_TYPE);
    _lbItems.importFromDlg(hDlg, IDC_ITEMS);
    _lbAttr.importFromDlg(hDlg, IDC_ATTRIBUTES);
    _readDlg = new ReadDialog(_hInstance);
    _writeDlg = new WriteDialog(_hInstance);
    HidFinder hidFinder;

    while (hidFinder.hasNext())
    {
        HidDevice *dev = hidFinder.next();
        _devList.push_back(dev);
    }

    _loadDevices();
    _loadItemTypes();

    // Post a message that the device changed so the appropriate
    //   data for the first device in the system can be displayed
    PostMessageA(hDlg, WM_COMMAND, IDC_DEVICES + (CBN_SELCHANGE<<16),
                LPARAM(GetDlgItem(hDlg, IDC_DEVICES)));

    return FALSE;
}


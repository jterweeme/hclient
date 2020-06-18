#ifndef MAINDLG_H
#define MAINDLG_H

#include "hclient.h"
#include "list.h"
#include <strsafe.h>

class ReadDialog;
class WriteDialog;

struct DEVICE_LIST_NODE
{
    LIST_ENTRY Hdr;
    HDEVNOTIFY NotificationHandle;
    HID_DEVICE HidDeviceInfo;
    HidDevice hidDevice;
    BOOL DeviceOpened;
};

class MainDialog
{
private:
    static MainDialog *_instance;
    HINSTANCE _hInstance;
    static INT_PTR CALLBACK bMainDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static VOID DestroyDeviceListCallback(LIST_ENTRY *ListNode);
    INT_PTR _command(HWND hDlg, WPARAM wParam);
    INT_PTR dlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static LRESULT addStrToCtrl(HWND hCtrl, STRSAFE_LPCSTR pszFormat, ...);
    void _displayDeviceAttributes(HIDD_ATTRIBUTES *pAttrib, HWND hControl);
    void _loadItemTypes(HWND hItemTypes);
    void _displayOutputButtons(HidDevice *pDevice, HWND hControl);
    void _displayInputButtons(HidDevice *pDevice, HWND hControl);
    void _displayValueAttributes(HIDP_VALUE_CAPS *pValue, HWND hControl);
    void _displayButtonAttributes(HIDP_BUTTON_CAPS *pButton, HWND hControl);
    void _displayDeviceCaps(HIDP_CAPS *pCaps, HWND hControl);
    void _displayFeatureButtons(HidDevice *pDevice, HWND hControl);
    void _displayFeatureValues(HidDevice *pDevice, HWND hControl);
    void _displayInputValues(HidDevice *pDevice, HWND hCtrl);
    void _displayOutputValues(HidDevice *pDevice, HWND hControl);
    void _loadDevices(HWND hDeviceCombo);
    BOOL _bGetData(rWriteDataStruct *pItems, INT iCount, HWND hParent);
    BOOL _setButtonUsages(HID_DATA *pCap, LPSTR pszInputString);
    BOOL _parseData(HID_DATA *pData, rWriteDataStruct rWriteData[], int iCount, int *piErrorLine);
    BOOL _registerHidDevice(HWND WindowHandle, DEVICE_LIST_NODE *DeviceNode);

    INT _prepareDataFields(HID_DATA *pData, ULONG ulDataLength,
        rWriteDataStruct rWriteData[], int iMaxElements);

    HidDevice *_getCurrentDevice(HWND hDlg);
    BOOLEAN _FindKnownHidDevices(HID_DEVICE **HidDevices, ULONG *nDevices);
    INT_PTR _init(HWND hDlg);
    INT_PTR _write(HWND hDlg);
    INT_PTR _read(HWND hDlg);
    INT_PTR _typeProc(HWND hDlg, WPARAM wParam);
    INT_PTR _itemsProc(HWND hDlg, WPARAM wParam);
    INT_PTR _caps(HWND hdlg);
    INT_PTR _devAttr(HWND hDlg);
    INT_PTR _devices(HWND hDlg, WPARAM wParam);
    INT_PTR _devArrival(HWND hDlg, LPARAM lParam);
    INT_PTR _devQuery(LPARAM lParam);
    INT_PTR _devRemove(HWND hDlg, LPARAM lParam);
    ReadDialog *_readDlg;
    WriteDialog *_writeDlg;
    CList g_physDevList;
    rWriteDataStruct rWriteData[MAX_OUTPUT_ELEMENTS];
public:
    MainDialog(HINSTANCE hInstance);
    ~MainDialog();
    void create();
};
#endif


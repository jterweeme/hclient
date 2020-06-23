#ifndef MAINDLG_H
#define MAINDLG_H

#include "hclient.h"
#include "hid.h"
#include "list.h"
#include "combobox.h"
#include "listbox.h"
#include "writedlg.h"
#include <strsafe.h>
#include <vector>

#define MAX_OUTPUT_ELEMENTS 50

class ReadDialog;
class WriteDialog;

struct DEVICE_LIST_NODE
{
    LIST_ENTRY Hdr;
    HDEVNOTIFY NotificationHandle;
    HID_DEVICE HidDeviceInfo;
    HidDevice *hidDevice;
    BOOL DeviceOpened;
};

class MainDialog
{
private:
    static constexpr INT INPUT_BUTTON = 1, INPUT_VALUE = 2, OUTPUT_BUTTON = 3,
        OUTPUT_VALUE = 4, FEATURE_BUTTON = 5, FEATURE_VALUE = 6, HID_CAPS = 7;

    HWND _hDlg;
    static MainDialog *_instance;
    HINSTANCE _hInstance;
    static INT_PTR CALLBACK bMainDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static VOID DestroyDeviceListCallback(LIST_ENTRY *ListNode);
    INT_PTR _command(HWND hDlg, WPARAM wParam);
    INT_PTR dlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    Combobox _cbDevices;
    Combobox _cbItemType;
    Listbox _lbItems;
    Listbox _lbAttr;
    rWriteDataStruct _rWriteData[MAX_OUTPUT_ELEMENTS];
    std::vector<WriteData> _writeData;
    void _loadItemTypes();
    void _loadDevices();
    BOOL _bGetData();
    BOOL _bGetData(INT iCount);
    BOOL _setButtonUsages(HID_DATA *pCap, LPSTR pszInputString);
    BOOL _parseData(HID_DATA *pData, rWriteDataStruct *rWriteData, int iCount, int *piErrorLine);
    BOOL _registerHidDevice(HWND WindowHandle, DEVICE_LIST_NODE *DeviceNode) const;
    ULONG _prepareDataFields2(HID_DATA *pData, ULONG len);
    ULONG _prepareDataFields(HID_DATA *pData, ULONG len);
    HidDevice *_getCurrentDevice() const;
    INT_PTR _init(HWND hDlg);
    INT_PTR _write();
    INT_PTR _read(HWND hDlg);
    INT_PTR _typeProc(HWND hDlg, WPARAM wParam);
    INT_PTR _itemsProc(WPARAM wParam);
    INT_PTR _caps();
    INT_PTR _devAttr();
    INT_PTR _devices(HWND hDlg, WPARAM wParam);
    INT_PTR _devArrival(HWND hDlg, LPARAM lParam);
    INT_PTR _devQuery(LPARAM lParam);
    INT_PTR _devRemove(HWND hDlg, LPARAM lParam);
    ReadDialog *_readDlg;
    WriteDialog *_writeDlg;
    CList g_physDevList;
public:
    MainDialog(HINSTANCE hInstance);
    ~MainDialog();
    void create();
};
#endif


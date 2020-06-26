#ifndef MAINDLG_H
#define MAINDLG_H

#include "hclient.h"
#include "hid.h"
#include "combobox.h"
#include "listbox.h"
#include "writedlg.h"
#include <strsafe.h>

#define MAX_OUTPUT_ELEMENTS 50

class ReadDialog;
class WriteDialog;

class MainDialog
{
private:
    static constexpr INT INPUT_BUTTON = 1, INPUT_VALUE = 2, OUTPUT_BUTTON = 3,
        OUTPUT_VALUE = 4, FEATURE_BUTTON = 5, FEATURE_VALUE = 6, HID_CAPS = 7,
        DEVICE_ATTRIBUTES = 8;

    static constexpr UINT WM_UNREGISTER_HANDLE = WM_USER + 1;
    HWND _hDlg;
    static MainDialog *_instance;
    HINSTANCE _hInstance;
    static INT_PTR CALLBACK bMainDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    INT_PTR _command(HWND hDlg, WPARAM wParam);
    INT_PTR dlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    Combobox _cbDevices;
    Combobox _cbItemType;
    Listbox _lbItems;
    Listbox _lbAttr;
    std::vector<WriteData> _writeData;
    void _loadItemTypes();
    void _loadDevices();
    INT _bGetData();
    void _setButtonUsages(HidData *pCap, LPCSTR pszInputString);
    BOOL _parseData(HidData *pData, int iCount);
    ULONG _prepareDataFields2(HidData *pData, ULONG len);
    HidDevice *_getCurrentDevice() const;
    INT_PTR _init(HWND hDlg);
    INT_PTR _write();
    INT_PTR _read();
    INT_PTR _typeProc(HWND hDlg, WPARAM wParam);
    INT_PTR _itemsProc(WPARAM wParam);
    INT_PTR _devices(HWND hDlg, WPARAM wParam);
    ReadDialog *_readDlg;
    WriteDialog *_writeDlg;
    std::vector<HidDevice *> _devList;
public:
    MainDialog(HINSTANCE hInstance);
    ~MainDialog();
    void create();
};
#endif


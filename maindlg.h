#ifndef MAINDLG_H
#define MAINDLG_H

#include "hclient.h"
#include <windows.h>

class ReadDialog;
class WriteDialog;

class MainDialog
{
private:
    static MainDialog *_instance;
    HINSTANCE _hInstance;
    static INT_PTR CALLBACK bMainDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    INT_PTR dlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
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
    BOOLEAN init(HWND hDlg);
    ReadDialog *_readDlg;
    WriteDialog *_writeDlg;
public:
    MainDialog(HINSTANCE hInstance);
    virtual ~MainDialog();
    void create();
};
#endif


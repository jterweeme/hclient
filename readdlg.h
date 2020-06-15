#ifndef READDLG_H
#define READDLG_H

#include "hid.h"

class ReadDialog
{
private:
    HINSTANCE _hInst;
    static ReadDialog *_instance;
    static INT_PTR CALLBACK _readDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    INT_PTR CALLBACK _bReadDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    //VOID _reportToString(HID_DATA *pData, LPSTR szBuff, UINT iBuffSize);
public:
    ReadDialog(HINSTANCE hInst);
    void create(HWND hDlg, HidDevice *dev);
};

#endif


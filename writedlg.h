#ifndef WRITEDLG_H
#define WRITEDLG_H

#include "hclient.h"
#include <windows.h>

class WriteDialog
{
private:
    HINSTANCE _hInst;
    static WriteDialog *_instance;
    static INT_PTR CALLBACK _dlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp);
    INT_PTR _bGetDataDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp);
    VOID _writeDataToControls(HWND hDlg, rWriteDataStruct *prData, INT iOffset, INT iCount);
    VOID _readDataFromControls(HWND hDlg, rWriteDataStruct *prData, INT iOffset, INT iCount);
public:
    WriteDialog(HINSTANCE hInstance);
    INT create(HWND parent, rGetWriteDataParams *rParams);
};
#endif


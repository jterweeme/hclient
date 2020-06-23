#ifndef WRITEDLG_H
#define WRITEDLG_H

#include "hclient.h"
#include <windows.h>
#include <iostream>

struct WriteData
{
    std::string label;
    std::string value;
    void dump(std::ostream &os) const;
};

struct rWriteDataStruct
{
    char szLabel[MAX_LABEL];
    char szValue[MAX_VALUE];
};

class WriteDialog
{
private:
    HWND _hDlg;
    rWriteDataStruct *_prData;
    int _prCount;
    INT iDisplayCount;
    INT iScrollRange;
    INT iCurrentScrollPos = 0;
    HWND hScrollBar;
    static const INT CONTROL_COUNT = 9;
    HINSTANCE _hInst;
    static WriteDialog *_instance;
    static INT_PTR CALLBACK _dlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp);
    INT_PTR _bGetDataDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp);
    VOID _writeDataToControls(INT iOffset, INT iCount);
    VOID _readDataFromControls(INT iOffset, INT iCount);
    INT_PTR _initDialog(HWND hDlg);
public:
    WriteDialog(HINSTANCE hInstance);
    INT create(HWND parent, rWriteDataStruct *data, int count);
};
#endif


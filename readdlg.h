#ifndef READDLG_H
#define READDLG_H

#include "hid.h"

struct READ_THREAD_CONTEXT
{
    HidDevice *hidDevice2;
    HWND DisplayWindow;
    HANDLE DisplayEvent;
    ULONG NumberOfReads;
    BOOL TerminateThread;
};

class ReadDialog
{
private:
    HINSTANCE _hInst;
    static ReadDialog *_instance;
    static INT_PTR CALLBACK _readDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    INT_PTR CALLBACK _bReadDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    READ_THREAD_CONTEXT readContext;
    static DWORD WINAPI AsynchReadThreadProc(READ_THREAD_CONTEXT *Context);
    INT _lbCounter;
    HANDLE readThread;
    HidDevice _syncDevice2;
    HidDevice _asyncDevice2;
    BOOL _doAsyncReads;
    BOOL _doSyncReads;
    static constexpr INT MAX_LB_ITEMS = 200;
public:
    ReadDialog(HINSTANCE hInst);
    void create(HWND hDlg, HidDevice *dev);
};

#endif


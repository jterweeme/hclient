#ifndef READDLG_H
#define READDLG_H

#include "hid.h"
#include "listbox.h"
#include "button.h"

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
    static constexpr UINT WM_DISPLAY_READ_DATA = WM_USER + 2;
    static constexpr UINT WM_READ_DONE = WM_USER + 3;
    HidDevice *_dev;
    Listbox _lbOutput;
    Button _btnCont;
    Button _btnOnce;
    Button _btnSync;
    Button _btnOk;
    HINSTANCE _hInst;
    static ReadDialog *_instance;
    static INT_PTR CALLBACK _readDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    INT_PTR _bReadDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    INT_PTR _initDialog(HWND hDlg);
    INT_PTR _command(HWND hDlg, WPARAM wParam);
    INT_PTR _readAsync(HWND hDlg, WPARAM wParam);
    INT_PTR _display(LPARAM lp);
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
    ~ReadDialog();
    void create(HWND hDlg, HidDevice *dev);
};

#endif


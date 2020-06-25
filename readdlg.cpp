#include "readdlg.h"
#include "hclient.h"
#include "resource.h"
#include "hidinfo.h"
#include "toolbox.h"
#include <strsafe.h>
#include <iostream>

ReadDialog *ReadDialog::_instance;

ReadDialog::ReadDialog(HINSTANCE hInst) : _hInst(hInst)
{
    _instance = this;
}

ReadDialog::~ReadDialog()
{
}

INT_PTR CALLBACK ReadDialog::_readDlgProc(
    HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    return _instance->_bReadDlgProc(hDlg, message, wParam, lParam);
}

void ReadDialog::create(HWND hParent, HidDevice *dev)
{
    _dev = dev;
    INT_PTR ret = DialogBoxParamA(_hInst, "READDATA", hParent, _readDlgProc, 0);

    if (ret < 0)
        throw "Error creating ReadDialog";
}

#define INFINITE_READS ((ULONG)-1)

DWORD WINAPI ReadDialog::AsynchReadThreadProc(READ_THREAD_CONTEXT *Context)
{
    BOOL readResult;
    DWORD waitStatus;
    ULONG numReadsDone;
    OVERLAPPED overlap;
    DWORD bytesTransferred;

    // Create the completion event to send to the the OverlappedRead routine
    HANDLE completionEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    HidDevice *hidDevice = Context->hidDevice2;
    //HID_DEVICE *tmp = hidDevice->getp();

    if (completionEvent == NULL)
        goto AsyncRead_End;

    numReadsDone = 0;

    do
    {
        // Call ReadOverlapped() and if the return status is TRUE, the ReadFile
        //  succeeded so we need to block on completionEvent, otherwise, we just
        //  exit
        readResult = hidDevice->readOverlapped(completionEvent, &overlap);

        if (!readResult)
            break;

        while (!Context->TerminateThread)
        {
            waitStatus = WaitForSingleObject(completionEvent, 1000);

            if (waitStatus == WAIT_OBJECT_0)
            {
                readResult = GetOverlappedResult(hidDevice->handle(),
                                       &overlap, &bytesTransferred, TRUE);
                break;
            }
        }

        if (!Context->TerminateThread)
        {
            numReadsDone++;
#if 0
            UnpackReport(tmp->InputReportBuffer, tmp->Caps.InputReportByteLength,
                     HidP_Input, tmp->InputData, tmp->InputDataLength, tmp->Ppd);


            if (Context->DisplayEvent != NULL)
            {
                PostMessageA(Context->DisplayWindow, WM_DISPLAY_READ_DATA, 0, LPARAM(tmp));
                WaitForSingleObject(Context->DisplayEvent, INFINITE);
            }
            else if (Context->DisplayWindow != NULL)
            {
                HID_DEVICE *pDevice = tmp;

                HID_DATA *pData = pDevice->InputData;

                SendMessageA(GetDlgItem(Context->DisplayWindow, IDC_CALLOUTPUT),
                        LB_ADDSTRING, 0,
                        LPARAM("-------------------------------------------"));

                for (UINT uLoop = 0; uLoop < pDevice->InputDataLength; uLoop++)
                {
                    std::string foo = HidInfo().report(pData);

                    Listbox lb;
                    lb.importFromDlg(Context->DisplayWindow, IDC_CALLOUTPUT);
                    lb.addStr(foo.c_str());
                    pData++;
                }

            }
#endif
        }
    } while (readResult && !Context->TerminateThread &&
              (INFINITE_READS == Context->NumberOfReads ||
              numReadsDone < Context->NumberOfReads));

AsyncRead_End:
    PostMessageA(Context->DisplayWindow, WM_READ_DONE, 0, 0);
    ExitThread(0);
    return (0);
}

INT_PTR ReadDialog::_initDialog(HWND hDlg)
{
    _btnCont.importFromDlg(hDlg, IDC_READ_ASYNCH_CONT);
    _btnOnce.importFromDlg(hDlg, IDC_READ_ASYNCH_ONCE);
    _btnSync.importFromDlg(hDlg, IDC_READ_SYNCH);
    _btnOk.importFromDlg(hDlg, IDOK);
    _lbOutput.importFromDlg(hDlg, IDC_OUTPUT);
    _lbCounter = 0;
    readThread = NULL;
    readContext.DisplayEvent = CreateEventA(NULL, FALSE, FALSE, NULL);

    if (readContext.DisplayEvent == NULL)
        EndDialog(hDlg, 0);

    std::cout << _dev->devicePath() << "a\n";
    std::cout.flush();
    _doSyncReads = _syncDevice2.open(_dev->devicePath(), TRUE, FALSE, FALSE, FALSE);

    if (!_doSyncReads)
        Toolbox().errorbox(hDlg, "Unable to open device for synchronous reading");

    _doAsyncReads = _asyncDevice2.open(_dev->devicePath(), TRUE, FALSE, TRUE, FALSE);

    if (!_doAsyncReads)
        Toolbox().errorbox(hDlg, "Unable to open device for asynchronous reading");

    PostMessageA(hDlg, WM_READ_DONE, 0, 0);
    return FALSE;
}

INT_PTR ReadDialog::_display(LPARAM lp)
{
    HidDevice *pDevice2 = reinterpret_cast<HidDevice *>(lp);
    HID_DATA *pData = pDevice2->inputData();
    _lbOutput.addStr("-------------------------------------------");
    _lbCounter++;

    if (_lbCounter > MAX_LB_ITEMS)
        _lbOutput.sendMsgA(LB_DELETESTRING, 0, 0);

    ULONG dataLen = pDevice2->input()->dataLength();
    for (UINT uLoop = 0; uLoop < dataLen; uLoop++)
    {
        std::string s = HidInfo().report(pData);
        INT iIndex = INT(_lbOutput.addStr(s.c_str()));
        _lbOutput.sendMsgA(LB_SETCURSEL, iIndex, 0);
        _lbCounter++;

        if (_lbCounter > MAX_LB_ITEMS)
            _lbOutput.sendMsgA(LB_DELETESTRING, 0, 0);

        pData++;
    }
    SetEvent(readContext.DisplayEvent);
    return FALSE;
}

INT_PTR ReadDialog::_readAsync(HWND hDlg, WPARAM wParam)
{
    if (readThread == NULL)
    {
        readContext.hidDevice2 = &_asyncDevice2;
        readContext.TerminateThread = FALSE;
        readContext.NumberOfReads = IDC_READ_ASYNCH_ONCE == LOWORD(wParam) ? 1 : INFINITE_READS;
        readContext.DisplayWindow = hDlg;
        DWORD threadID;

        readThread = CreateThread(NULL, 0, LPTHREAD_START_ROUTINE(AsynchReadThreadProc),
                                    &readContext, 0, &threadID);

        if (readThread == NULL)
        {
            Toolbox().errorbox(hDlg, "Unable to create read thread");
            return FALSE;
        }

        _btnOk.disable();
        _btnSync.disable();
        _btnOnce.enable(LOWORD(wParam) == IDC_READ_ASYNCH_ONCE);
        _btnCont.enable(LOWORD(wParam) == IDC_READ_ASYNCH_CONT);
        SetWindowTextA(GetDlgItem(hDlg, LOWORD(wParam)), "Stop Asynchronous Read");
    }
    else
    {
        readContext.TerminateThread = TRUE;
        WaitForSingleObject(readThread, INFINITE);
    }
    return FALSE;
}

INT_PTR ReadDialog::_command(HWND hDlg, WPARAM wParam)
{
    switch(LOWORD(wParam))
    {
    case IDC_READ_SYNCH:
        _btnOk.disable();
        _btnSync.disable();
        _btnOnce.disable();
        _btnCont.disable();
        _syncDevice2.read();
        PostMessageA(hDlg, WM_DISPLAY_READ_DATA, 0, LPARAM(&_syncDevice2));
        PostMessageA(hDlg, WM_READ_DONE, 0, 0);
        return FALSE;
    case IDC_READ_ASYNCH_ONCE:
    case IDC_READ_ASYNCH_CONT:
        return _readAsync(hDlg, wParam);
    case IDCANCEL:
        readContext.TerminateThread = TRUE;
        WaitForSingleObject(readThread, INFINITE);
        //Fall through!!!
    case IDOK:
        _asyncDevice2.close();
        _syncDevice2.close();
        EndDialog(hDlg, 0);
        return FALSE;
    }
    return FALSE;
}

INT_PTR ReadDialog::_bReadDlgProc(
    HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        return _initDialog(hDlg);
    case WM_DISPLAY_READ_DATA:
        return _display(lParam);
    case WM_READ_DONE:
        _btnOk.enable();
        _btnSync.enable(_doSyncReads);
        _btnOnce.enable(_doAsyncReads);
        _btnCont.enable(_doAsyncReads);
        _btnOnce.text("Once Asynchronous Read");
        _btnCont.text("Continuous Asynchronous Read");
        readThread = NULL;
        break;
    case WM_COMMAND:
        return _command(hDlg, wParam);
    }
    return FALSE;
}


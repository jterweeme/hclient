#include "readdlg.h"
#include "hclient.h"
#include "resource.h"
#include <strsafe.h>

ReadDialog *ReadDialog::_instance;

ReadDialog::ReadDialog(HINSTANCE hInst) : _hInst(hInst)
{
    _instance = this;
}

INT_PTR CALLBACK ReadDialog::_readDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    return _instance->_bReadDlgProc(hDlg, message, wParam, lParam);
}

void ReadDialog::create(HWND hParent, HidDevice *dev)
{
    INT_PTR ret = DialogBoxParamA(_hInst, "READDATA", hParent, _readDlgProc, LPARAM(dev->getp()));

    if (ret < 0)
        throw "Error creating ReadDialog";
}

static VOID ReportToString(HID_DATA *pData, LPSTR szBuff, UINT iBuffSize)
{
    PCHAR pszWalk;
    PUSAGE pUsage;
    ULONG i;
    UINT iRemainingBuffer;
    UINT iStringLength;
    UINT j;

    // For button data, all the usages in the usage list are to be displayed
    if (pData->IsButtonData)
    {
        if (StringCbPrintfA(szBuff, iBuffSize, "Usage Page: 0x%x, Usages: ",
                            pData->UsagePage) < 0)
        {
            for (j = 0; j < iBuffSize; j++)
                szBuff[j] = '\0';

            return;  // error case
        }

        iRemainingBuffer = 0;
        iStringLength = (UINT)strlen(szBuff);
        pszWalk = szBuff + iStringLength;

        if (iStringLength < iBuffSize)
            iRemainingBuffer = iBuffSize - iStringLength;

        for (i = 0, pUsage = pData->ButtonData.Usages;
                     i < pData -> ButtonData.MaxUsageLength;
                         i++, pUsage++)
        {
            if (0 == *pUsage)
            {
                break; // A usage of zero is a non button.
            }

            if (StringCbPrintfA(pszWalk, iRemainingBuffer, " 0x%x", *pUsage) < 0)
            {
                return; // error case
            }
            iRemainingBuffer -= (UINT)strlen(pszWalk);
            pszWalk += strlen(pszWalk);
        }
    }
    else
    {
        if (FAILED(StringCbPrintfA(szBuff, iBuffSize,
                        "Usage Page: 0x%x, Usage: 0x%x, Scaled: %d Value: %d",
                        pData->UsagePage,
                        pData->ValueData.Usage,
                        pData->ValueData.ScaledValue,
                        pData->ValueData.Value)))
        {
            return; // error case
        }
    }
}

#define READ_THREAD_TIMEOUT 1000
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
    HID_DEVICE *tmp = hidDevice->getp();

    // If NULL returned, then we cannot proceed any farther so we just exit the
    //  the thread
    if (NULL == completionEvent)
        goto AsyncRead_End;

    // Now we enter the main read loop, which does the following:
    //  1) Calls ReadOverlapped()
    //  2) Waits for read completion with a timeout just to check if
    //      the main thread wants us to terminate our the read request
    //  3) If the read fails, we simply break out of the loop
    //      and exit the thread
    //  4) If the read succeeds, we call UnpackReport to get the relevant
    //      info and then post a message to main thread to indicate that
    //      there is new data to display.
    //  5) We then block on the display event until the main thread says
    //      it has properly displayed the new data
    //  6) Look to repeat this loop if we are doing more than one read
    //      and the main thread has yet to want us to terminate
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
            // Wait for the completion event to be signaled or a timeout
            waitStatus = WaitForSingleObject(completionEvent, READ_THREAD_TIMEOUT );

            // If completionEvent was signaled, then a read just completed
            //   so let's get the status and leave this loop and process the data
            if (waitStatus == WAIT_OBJECT_0)
            {
                readResult = GetOverlappedResult(tmp->HidDevice, &overlap, &bytesTransferred, TRUE);
                break;
            }
        }

        // Check the TerminateThread again...If it is not set, then data has
        //  been read.  In this case, we want to Unpack the report into our
        //  input info and then send a message to the main thread to display
        //  the new data.
        if (!Context->TerminateThread)
        {
            numReadsDone++;

            UnpackReport(tmp->InputReportBuffer, tmp->Caps.InputReportByteLength,
                     HidP_Input, tmp->InputData, tmp->InputDataLength, tmp->Ppd);

            if (Context->DisplayEvent != NULL)
            {
                PostMessageA(Context->DisplayWindow, WM_DISPLAY_READ_DATA, 0, LPARAM(tmp));
                WaitForSingleObject(Context->DisplayEvent, INFINITE);
            }
            else if (NULL != Context->DisplayWindow)
            {
                CHAR szTempBuff[1024];
                HID_DEVICE *pDevice = tmp;

                // Display all the data stored in the Input data field for the device
                HID_DATA *pData = pDevice->InputData;

                SendMessageA(GetDlgItem(Context->DisplayWindow, IDC_CALLOUTPUT),
                    LB_ADDSTRING, 0, LPARAM("-------------------------------------------"));

                for (UINT uLoop = 0; uLoop < pDevice->InputDataLength; uLoop++)
                {
                    ReportToString(pData, szTempBuff, sizeof(szTempBuff));

                    SendMessageA(GetDlgItem(Context->DisplayWindow, IDC_CALLOUTPUT),
                        LB_ADDSTRING, 0, LPARAM(szTempBuff));

                    pData++;
                }
            }
        }
    } while (readResult && !Context->TerminateThread &&
              (INFINITE_READS == Context->NumberOfReads ||
              numReadsDone < Context->NumberOfReads));

AsyncRead_End:
    PostMessageA(Context->DisplayWindow, WM_READ_DONE, 0, 0);
    ExitThread(0);
    return (0);
}

INT_PTR CALLBACK
ReadDialog::_bReadDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
    {
        // Initialize the list box counter, the readThread, and the
        //  readContext.DisplayEvent.
        _lbCounter = 0;
        readThread = NULL;
        readContext.DisplayEvent = CreateEventA(NULL, FALSE, FALSE, NULL);

        if (readContext.DisplayEvent == NULL)
            EndDialog(hDlg, 0);

        // Get the opened device information for the device to perform
        //  reads upon
        HID_DEVICE *pDevice = reinterpret_cast<HID_DEVICE *>(lParam);

        // To do sync and async reads requires file handles with different
        //  attributes (ie. an async must be opened with the OVERLAPPED flag
        //  set).  The device node that was passed in the context parameter
        //  was not opened for reading.  Therefore, two more devices will
        //  be opened, one for async reads and one for sync reads.
        _doSyncReads = _syncDevice2.open(pDevice->DevicePath, TRUE, FALSE, FALSE, FALSE);

        if (!_doSyncReads)
        {
            MessageBoxA(hDlg, "Unable to open device for synchronous reading",
                       HCLIENT_ERROR, MB_ICONEXCLAMATION);
        }

        // For asynchronous read, default to using the same information
        //    passed in as the lParam.  This is because data related to
        //    Ppd and such cannot be retrieved using the standard HidD_
        //    functions.  However, it is necessary to parse future reports.
        _doAsyncReads = _asyncDevice2.open(pDevice->DevicePath, TRUE, FALSE, TRUE, FALSE);

        if (!_doAsyncReads)
        {
            MessageBoxA(hDlg, "Unable to open device for asynchronous reading",
                        HCLIENT_ERROR, MB_ICONEXCLAMATION);
        }

        PostMessageA(hDlg, WM_READ_DONE, 0, 0);
    }
        break;
    case WM_DISPLAY_READ_DATA:
    {
        // LParam is the device that was read from
        HidDevice *pDevice2 = reinterpret_cast<HidDevice *>(lParam);
        //HID_DEVICE *pDevice = reinterpret_cast<HID_DEVICE *>(lParam);
        HID_DEVICE *pDevice = pDevice2->getp();

        // Display all the data stored in the Input data field for the device
        HID_DATA *pData = pDevice->InputData;

        SendDlgItemMessageA(hDlg, IDC_OUTPUT, LB_ADDSTRING, 0,
                           LPARAM("-------------------------------------------"));

        _lbCounter++;

        if (_lbCounter > MAX_LB_ITEMS)
            SendDlgItemMessageA(hDlg, IDC_OUTPUT, LB_DELETESTRING, 0, 0);

        for (UINT uLoop = 0; uLoop < pDevice->InputDataLength; uLoop++)
        {
            CHAR szTempBuff[1024];
            ReportToString(pData, szTempBuff, sizeof(szTempBuff));
            INT iIndex = INT(SendDlgItemMessageA(hDlg, IDC_OUTPUT, LB_ADDSTRING, 0, LPARAM(szTempBuff)));
            SendDlgItemMessage(hDlg, IDC_OUTPUT, LB_SETCURSEL, iIndex, 0);
            _lbCounter++;

            if (_lbCounter > MAX_LB_ITEMS)
                SendDlgItemMessageA(hDlg, IDC_OUTPUT, LB_DELETESTRING, 0, 0);

            pData++;
        }
        SetEvent(readContext.DisplayEvent);
    }
        break;
    case WM_READ_DONE:
        EnableWindow(GetDlgItem(hDlg, IDOK), TRUE);
        EnableWindow(GetDlgItem(hDlg, IDC_READ_SYNCH), _doSyncReads);
        EnableWindow(GetDlgItem(hDlg, IDC_READ_ASYNCH_ONCE), _doAsyncReads);
        EnableWindow(GetDlgItem(hDlg, IDC_READ_ASYNCH_CONT), _doAsyncReads);

        SetWindowTextA(GetDlgItem(hDlg, IDC_READ_ASYNCH_ONCE),
                      "One Asynchronous Read");

        SetWindowTextA(GetDlgItem(hDlg, IDC_READ_ASYNCH_CONT),
                      "Continuous Asynchronous Read");

        readThread = NULL;
        break;
    case WM_COMMAND:
        switch(LOWORD(wParam))
        {
        case IDC_READ_SYNCH:
            EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_READ_SYNCH), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_READ_ASYNCH_ONCE), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_READ_ASYNCH_CONT), FALSE);
            _syncDevice2.read();
            PostMessageA(hDlg, WM_DISPLAY_READ_DATA, 0, LPARAM(&_syncDevice2));
            PostMessageA(hDlg, WM_READ_DONE, 0, 0);
            break;
        case IDC_READ_ASYNCH_ONCE:
        case IDC_READ_ASYNCH_CONT:
            // When these buttons are pushed there are two options:
            //  1) Start a new asynch read thread (readThread == NULL)
            //  2) Stop a previous asych read thread
            if (readThread == NULL)
            {
                // Start a new read thread
                readContext.hidDevice2 = &_asyncDevice2;
                readContext.TerminateThread = FALSE;
                readContext.NumberOfReads = (IDC_READ_ASYNCH_ONCE == LOWORD(wParam))?1:INFINITE_READS;
                readContext.DisplayWindow = hDlg;
                DWORD threadID;

                readThread = CreateThread(NULL, 0, LPTHREAD_START_ROUTINE(AsynchReadThreadProc),
                                            &readContext, 0, &threadID);

                if (NULL == readThread)
                {
                    MessageBoxA(hDlg, "Unable to create read thread", HCLIENT_ERROR, MB_ICONEXCLAMATION);
                }
                else
                {
                    EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
                    EnableWindow(GetDlgItem(hDlg, IDC_READ_SYNCH), FALSE);
                    EnableWindow(GetDlgItem(hDlg, IDC_READ_ASYNCH_ONCE), IDC_READ_ASYNCH_ONCE == LOWORD(wParam));
                    EnableWindow(GetDlgItem(hDlg, IDC_READ_ASYNCH_CONT), IDC_READ_ASYNCH_CONT == LOWORD(wParam));
                    SetWindowTextA(GetDlgItem(hDlg, LOWORD(wParam)), "Stop Asynchronous Read");
                }
            }
            else
            {
                // Signal the terminate thread variable and
                //  wait for the read thread to complete.
                readContext.TerminateThread = TRUE;
                WaitForSingleObject(readThread, INFINITE);
            }
            break;
        case IDCANCEL:
            readContext.TerminateThread = TRUE;
            WaitForSingleObject(readThread, INFINITE);
            //Fall through!!!
        case IDOK:
            _asyncDevice2.close();
            _syncDevice2.close();
            EndDialog(hDlg, 0);
            break;
        }
        break;
    }
    return FALSE;
}


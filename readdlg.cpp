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

static DWORD WINAPI AsynchReadThreadProc(READ_THREAD_CONTEXT *Context)
{
    BOOL readResult;
    DWORD waitStatus;
    ULONG numReadsDone;
    OVERLAPPED overlap;
    DWORD bytesTransferred;

    // Create the completion event to send to the the OverlappedRead routine
    HANDLE completionEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    // If NULL returned, then we cannot proceed any farther so we just exit the
    //  the thread
    if (NULL == completionEvent)
        goto AsyncRead_End;

    //
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
    //

    numReadsDone = 0;

    do
    {
        // Call ReadOverlapped() and if the return status is TRUE, the ReadFile
        //  succeeded so we need to block on completionEvent, otherwise, we just
        //  exit
        readResult = ReadOverlapped(Context->HidDevice, completionEvent, &overlap);

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
                readResult = GetOverlappedResult(Context->HidDevice->HidDevice, &overlap, &bytesTransferred, TRUE);
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

            UnpackReport(Context->HidDevice->InputReportBuffer,
                          Context->HidDevice->Caps.InputReportByteLength,
                          HidP_Input, Context->HidDevice->InputData,
                          Context->HidDevice->InputDataLength, Context->HidDevice->Ppd);

            if (Context->DisplayEvent != NULL)
            {
                PostMessageA(Context->DisplayWindow, WM_DISPLAY_READ_DATA, 0, LPARAM(Context->HidDevice));
                WaitForSingleObject(Context->DisplayEvent, INFINITE);
            }
            else if (NULL != Context->DisplayWindow)
            {
                CHAR szTempBuff[1024];
                HID_DEVICE *pDevice;
                pDevice = reinterpret_cast<HID_DEVICE *>(Context->HidDevice);

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
    static INT iLbCounter;
    static CHAR szTempBuff[1024];
    static READ_THREAD_CONTEXT readContext;
    static HANDLE readThread;
    static HID_DEVICE syncDevice;
    static HID_DEVICE asyncDevice;
    static BOOL doAsyncReads;
    static BOOL doSyncReads;
    HID_DEVICE *pDevice;
    DWORD threadID;
    INT iIndex;
    HID_DATA *pData;
    UINT uLoop;

    switch (message)
    {
    case WM_INITDIALOG:
        // Initialize the list box counter, the readThread, and the
        //  readContext.DisplayEvent.
        iLbCounter = 0;
        readThread = NULL;
        readContext.DisplayEvent = CreateEventA(NULL, FALSE, FALSE, NULL);

        if (NULL == readContext.DisplayEvent)
            EndDialog(hDlg, 0);

        // Get the opened device information for the device to perform
        //  reads upon
        pDevice = reinterpret_cast<HID_DEVICE *>(lParam);

        // To do sync and async reads requires file handles with different
        //  attributes (ie. an async must be opened with the OVERLAPPED flag
        //  set).  The device node that was passed in the context parameter
        //  was not opened for reading.  Therefore, two more devices will
        //  be opened, one for async reads and one for sync reads.
        doSyncReads = OpenHidDevice(pDevice->DevicePath, TRUE,
                                   FALSE, FALSE, FALSE, &syncDevice);

        if (!doSyncReads)
        {
            MessageBoxA(hDlg, "Unable to open device for synchronous reading",
                       HCLIENT_ERROR, MB_ICONEXCLAMATION);
        }

        // For asynchronous read, default to using the same information
        //    passed in as the lParam.  This is because data related to
        //    Ppd and such cannot be retrieved using the standard HidD_
        //    functions.  However, it is necessary to parse future reports.
        doAsyncReads = OpenHidDevice(pDevice -> DevicePath,
                             TRUE, FALSE, TRUE, FALSE, &asyncDevice);

        if (!doAsyncReads)
        {
            MessageBoxA(hDlg, "Unable to open device for asynchronous reading",
                       HCLIENT_ERROR, MB_ICONEXCLAMATION);
        }

        PostMessageA(hDlg, WM_READ_DONE, 0, 0);
        break;
    case WM_DISPLAY_READ_DATA:
        // LParam is the device that was read from
        pDevice = reinterpret_cast<HID_DEVICE *>(lParam);

        // Display all the data stored in the Input data field for the device
        pData = pDevice->InputData;

        SendDlgItemMessageA(hDlg, IDC_OUTPUT, LB_ADDSTRING, 0,
                           LPARAM("-------------------------------------------"));

        iLbCounter++;

        if (iLbCounter > MAX_LB_ITEMS)
            SendDlgItemMessageA(hDlg, IDC_OUTPUT, LB_DELETESTRING, 0, 0);

        for (uLoop = 0; uLoop < pDevice->InputDataLength; uLoop++)
        {
            ReportToString(pData, szTempBuff, sizeof(szTempBuff));
            iIndex = INT(SendDlgItemMessageA(hDlg, IDC_OUTPUT, LB_ADDSTRING, 0, LPARAM(szTempBuff)));
            SendDlgItemMessage(hDlg, IDC_OUTPUT, LB_SETCURSEL, iIndex, 0);
            iLbCounter++;

            if (iLbCounter > MAX_LB_ITEMS)
                SendDlgItemMessageA(hDlg, IDC_OUTPUT, LB_DELETESTRING, 0, 0);

            pData++;
        }
        SetEvent(readContext.DisplayEvent);
        break;
    case WM_READ_DONE:
        EnableWindow(GetDlgItem(hDlg, IDOK), TRUE);
        EnableWindow(GetDlgItem(hDlg, IDC_READ_SYNCH), doSyncReads);
        EnableWindow(GetDlgItem(hDlg, IDC_READ_ASYNCH_ONCE), doAsyncReads);
        EnableWindow(GetDlgItem(hDlg, IDC_READ_ASYNCH_CONT), doAsyncReads);

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
            HidRead(&syncDevice);
            PostMessageA(hDlg, WM_DISPLAY_READ_DATA, 0, LPARAM(&syncDevice));
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
                readContext.HidDevice = &asyncDevice;
                readContext.TerminateThread = FALSE;
                readContext.NumberOfReads = (IDC_READ_ASYNCH_ONCE == LOWORD(wParam))?1:INFINITE_READS;
                readContext.DisplayWindow = hDlg;

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
            CloseHidDevice(&asyncDevice);
            CloseHidDevice(&syncDevice);
            EndDialog(hDlg,0);
            break;
        }
        break;
    }
    return FALSE;
}


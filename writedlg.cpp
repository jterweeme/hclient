#include "writedlg.h"
#include "hclient.h"
#include "resource.h"

WriteDialog *WriteDialog::_instance;

WriteDialog::WriteDialog(HINSTANCE hInst) : _hInst(hInst)
{
    _instance = this;
}

INT WriteDialog::create(HWND parent, rGetWriteDataParams *rParams)
{
    INT_PTR ret;
    ret = DialogBoxParamA(_hInst, "WRITEDATA", parent, _dlgProc, LPARAM(rParams));
    return INT(ret);
}

INT_PTR CALLBACK WriteDialog::_dlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
    return _instance->_bGetDataDlgProc(hDlg, msg, wp, lp);
}

VOID WriteDialog::_readDataFromControls(HWND hDlg, rWriteDataStruct *prData, INT iOffset, INT iCount)
{
    INT iValueControlID = IDC_OUT_EDIT1;
    rWriteDataStruct *pDataWalk = prData + iOffset;

    for (INT iLoop = 0; iLoop < iCount && iLoop < CONTROL_COUNT; iLoop++)
    {
        HWND hValueWnd = GetDlgItem(hDlg, iValueControlID);
        GetWindowTextA(hValueWnd, pDataWalk->szValue, MAX_VALUE);
        iValueControlID++;
        pDataWalk++;
    }
}

VOID WriteDialog::_writeDataToControls(HWND hDlg, rWriteDataStruct *prData, INT iOffset, INT iCount)
{
    INT iLoop;
    INT iLabelControlID = IDC_OUT_LABEL1;
    INT iValueControlID = IDC_OUT_EDIT1;
    rWriteDataStruct *pDataWalk = prData + iOffset;

    for (iLoop = 0; iLoop < iCount && iLoop < CONTROL_COUNT; iLoop++)
    {
         HWND hLabelWnd = GetDlgItem(hDlg, iLabelControlID);
         HWND hValueWnd = GetDlgItem(hDlg, iValueControlID);
         ShowWindow(hLabelWnd, SW_SHOW);
         ShowWindow(hValueWnd, SW_SHOW);
         SetWindowTextA(hLabelWnd, pDataWalk->szLabel);
         SetWindowTextA(hValueWnd, pDataWalk->szValue);
         iLabelControlID++;
         iValueControlID++;
         pDataWalk++;
    }

    // Hide the controls
    for (; iLoop < CONTROL_COUNT; iLoop++)
    {
        HWND hLabelWnd = GetDlgItem(hDlg, iLabelControlID);
        HWND hValueWnd = GetDlgItem(hDlg, iValueControlID);
        ShowWindow(hLabelWnd, SW_HIDE);
        ShowWindow(hValueWnd, SW_HIDE);
        iLabelControlID++;
        iValueControlID++;
    }
}

INT_PTR
WriteDialog::_bGetDataDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    static rWriteDataStruct *prData;
    static rGetWriteDataParams *pParams;
    static INT iDisplayCount;
    static INT iScrollRange;
    static INT iCurrentScrollPos = 0;
    static HWND hScrollBar;
    INT iTemp;
    SCROLLINFO rScrollInfo;

    switch (message)
    {
    case WM_INITDIALOG:
        pParams = reinterpret_cast<rGetWriteDataParams *>(lParam);
        prData = pParams->prItems;
        hScrollBar = GetDlgItem(hDlg, IDC_SCROLLBAR);

        if (pParams->iCount > CONTROL_COUNT)
        {
            iDisplayCount = CONTROL_COUNT;
            iScrollRange = pParams->iCount - CONTROL_COUNT;
            rScrollInfo.fMask = SIF_RANGE | SIF_POS;
            rScrollInfo.nPos = 0;
            rScrollInfo.nMin = 0;
            rScrollInfo.nMax = iScrollRange;
            rScrollInfo.cbSize = sizeof(rScrollInfo);
            rScrollInfo.nPage = CONTROL_COUNT;
            SetScrollInfo(hScrollBar, SB_CTL, &rScrollInfo, TRUE);
        }
        else
        {
            iDisplayCount=pParams->iCount;
            EnableWindow(hScrollBar,FALSE);
        }
        _writeDataToControls(hDlg, prData, 0, pParams->iCount);
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
        case ID_SEND:
            _readDataFromControls(hDlg, prData, iCurrentScrollPos, iDisplayCount);
            EndDialog(hDlg, 1);
            break;
        case IDCANCEL:
            EndDialog(hDlg, 0);
            break;
        }
        break;
    case WM_VSCROLL:
        _readDataFromControls(hDlg, prData, iCurrentScrollPos, iDisplayCount);

        switch (LOWORD(wParam))
        {
        case SB_LINEDOWN:
            ++iCurrentScrollPos;
            break;
        case SB_LINEUP:
            --iCurrentScrollPos;
            break;
        case SB_THUMBPOSITION:
            iCurrentScrollPos = HIWORD(wParam);
            //fallthrough!
        case SB_PAGEUP:
            iCurrentScrollPos -= CONTROL_COUNT;
            break;
        case SB_PAGEDOWN:
            iCurrentScrollPos += CONTROL_COUNT;
            break;
        }

        if (iCurrentScrollPos < 0)
            iCurrentScrollPos = 0;

        if (iCurrentScrollPos > iScrollRange)
            iCurrentScrollPos = iScrollRange;

        SendMessageA(hScrollBar, SBM_SETPOS, iCurrentScrollPos, TRUE);
        iTemp = LOWORD(wParam);

        if (iTemp == SB_LINEDOWN || iTemp == SB_LINEUP ||
            iTemp == SB_THUMBPOSITION || iTemp == SB_PAGEUP || iTemp==SB_PAGEDOWN)
        {
            _writeDataToControls(hDlg, prData, iCurrentScrollPos, iDisplayCount);
        }
        break;
    } 
    return FALSE;
}


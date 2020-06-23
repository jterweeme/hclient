#include "writedlg.h"
#include "resource.h"

WriteDialog *WriteDialog::_instance;

void WriteData::dump(std::ostream &os) const
{
    os << label << ": " << value << "\n";
}

WriteDialog::WriteDialog(HINSTANCE hInst) : _hInst(hInst)
{
    _instance = this;
}

INT WriteDialog::create(HWND parent, rWriteDataStruct *data, int count)
{
    INT_PTR ret;
    _prData = data;
    _prCount = count;
    ret = DialogBoxParamA(_hInst, "WRITEDATA", parent, _dlgProc, 0);
    return INT(ret);
}

INT_PTR CALLBACK WriteDialog::_dlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
    return _instance->_bGetDataDlgProc(hDlg, msg, wp, lp);
}

VOID WriteDialog::_readDataFromControls(INT iOffset, INT iCount)
{
    INT iValueControlID = IDC_OUT_EDIT1;
    rWriteDataStruct *pDataWalk = _prData + iOffset;

    for (INT iLoop = 0; iLoop < iCount && iLoop < CONTROL_COUNT; iLoop++)
    {
        HWND hValueWnd = GetDlgItem(_hDlg, iValueControlID);
        GetWindowTextA(hValueWnd, pDataWalk->szValue, MAX_VALUE);
        iValueControlID++;
        pDataWalk++;
    }
}

VOID WriteDialog::_writeDataToControls(INT iOffset, INT iCount)
{
    INT iLoop;
    INT iLabelControlID = IDC_OUT_LABEL1;
    INT iValueControlID = IDC_OUT_EDIT1;
    rWriteDataStruct *pDataWalk = _prData + iOffset;

    for (iLoop = 0; iLoop < iCount && iLoop < CONTROL_COUNT; iLoop++)
    {
         HWND hLabelWnd = GetDlgItem(_hDlg, iLabelControlID);
         HWND hValueWnd = GetDlgItem(_hDlg, iValueControlID);
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
        HWND hLabelWnd = GetDlgItem(_hDlg, iLabelControlID);
        HWND hValueWnd = GetDlgItem(_hDlg, iValueControlID);
        ShowWindow(hLabelWnd, SW_HIDE);
        ShowWindow(hValueWnd, SW_HIDE);
        iLabelControlID++;
        iValueControlID++;
    }
}

INT_PTR WriteDialog::_initDialog(HWND hDlg)
{
    _hDlg = hDlg;
    hScrollBar = GetDlgItem(hDlg, IDC_SCROLLBAR);

    if (_prCount > CONTROL_COUNT)
    {
        iDisplayCount = CONTROL_COUNT;
        iScrollRange = _prCount - CONTROL_COUNT;
        SCROLLINFO rScrollInfo;
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
        iDisplayCount = _prCount;
        EnableWindow(hScrollBar, FALSE);
    }

    _writeDataToControls(0, _prCount);
    return FALSE;
}

INT_PTR
WriteDialog::_bGetDataDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM)
{
    switch (message)
    {
    case WM_INITDIALOG:
        return _initDialog(hDlg);
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
        case ID_SEND:
            _readDataFromControls(iCurrentScrollPos, iDisplayCount);
            EndDialog(hDlg, 1);
            break;
        case IDCANCEL:
            EndDialog(hDlg, 0);
            break;
        }
        break;
    case WM_VSCROLL:
        _readDataFromControls(iCurrentScrollPos, iDisplayCount);

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
        INT iTemp = LOWORD(wParam);

        if (iTemp == SB_LINEDOWN || iTemp == SB_LINEUP ||
            iTemp == SB_THUMBPOSITION || iTemp == SB_PAGEUP || iTemp==SB_PAGEDOWN)
        {
            _writeDataToControls(iCurrentScrollPos, iDisplayCount);
        }
        break;
    } 
    return FALSE;
}


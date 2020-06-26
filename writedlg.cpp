#include "writedlg.h"
#include "resource.h"

WriteDialog *WriteDialog::_instance;

void WriteData::dump(std::ostream &os) const
{
    os << _label << ": " << _value << "\n";
}

void WriteData::setLabel(const char *s)
{
    _label = std::string(s);
}

void WriteData::setValue(const char *s)
{
    _value = std::string(s);
}

std::string WriteData::getLabel() const
{
    return _label;
}

std::string WriteData::getValue() const
{
    return _value;
}

WriteDialog::WriteDialog(HINSTANCE hInst) : _hInst(hInst)
{
    _instance = this;
}

INT_PTR WriteDialog::create2(HWND parent, std::vector<WriteData> *data)
{
    _data = data;
    _prCount = data->size();
    return DialogBoxParamA(_hInst, "WRITEDATA", parent, _dlgProc, 0);
}

INT_PTR CALLBACK WriteDialog::_dlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
    return _instance->_bGetDataDlgProc(hDlg, msg, wp, lp);
}

void WriteDialog::_readDataFromControls(INT iOffset, INT iCount)
{
    INT iValueControlID = IDC_OUT_EDIT1;

    for (size_t iLoop = 0; iLoop < iCount && iLoop < CONTROL_COUNT; iLoop++)
    {
        HWND hValueWnd = GetDlgItem(_hDlg, iValueControlID);
        char buf[MAX_VALUE];
        GetWindowTextA(hValueWnd, buf, MAX_VALUE);
        _data->at(iLoop + iOffset).setValue(buf);
        iValueControlID++;
    }
}

void WriteDialog::_writeDataToControls(INT iOffset, size_t iCount)
{
    INT iLabelControlID = IDC_OUT_LABEL1;
    INT iValueControlID = IDC_OUT_EDIT1;

    size_t iLoop;
    for (iLoop = 0; iLoop < iCount && iLoop < CONTROL_COUNT; iLoop++)
    {
         HWND hLabelWnd = GetDlgItem(_hDlg, iLabelControlID);
         HWND hValueWnd = GetDlgItem(_hDlg, iValueControlID);
         ShowWindow(hLabelWnd, SW_SHOW);
         ShowWindow(hValueWnd, SW_SHOW);
         SetWindowTextA(hLabelWnd, _data->at(iLoop + iOffset).getLabel().c_str());
         SetWindowTextA(hValueWnd, _data->at(iLoop + iOffset).getValue().c_str());
         iLabelControlID++;
         iValueControlID++;
    }

    // Hide the unused controls
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
    _hScrollBar = GetDlgItem(hDlg, IDC_SCROLLBAR);

    if (_prCount > CONTROL_COUNT)
    {
        _iDisplayCount = CONTROL_COUNT;
        _iScrollRange = _prCount - CONTROL_COUNT;
        SCROLLINFO rScrollInfo;
        rScrollInfo.fMask = SIF_RANGE | SIF_POS;
        rScrollInfo.nPos = 0;
        rScrollInfo.nMin = 0;
        rScrollInfo.nMax = _iScrollRange;
        rScrollInfo.cbSize = sizeof(rScrollInfo);
        rScrollInfo.nPage = CONTROL_COUNT;
        SetScrollInfo(_hScrollBar, SB_CTL, &rScrollInfo, TRUE);
    }
    else
    {
        _iDisplayCount = _prCount;
        EnableWindow(_hScrollBar, FALSE);
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
            _readDataFromControls(_iCurrentScrollPos, _iDisplayCount);
            EndDialog(hDlg, 1);
            break;
        case IDCANCEL:
            EndDialog(hDlg, 0);
            break;
        }
        break;
    case WM_VSCROLL:
        _readDataFromControls(_iCurrentScrollPos, _iDisplayCount);

        switch (LOWORD(wParam))
        {
        case SB_LINEDOWN:
            ++_iCurrentScrollPos;
            break;
        case SB_LINEUP:
            --_iCurrentScrollPos;
            break;
        case SB_THUMBPOSITION:
            _iCurrentScrollPos = HIWORD(wParam);
            //fallthrough!
        case SB_PAGEUP:
            _iCurrentScrollPos -= CONTROL_COUNT;
            break;
        case SB_PAGEDOWN:
            _iCurrentScrollPos += CONTROL_COUNT;
            break;
        }

        _iCurrentScrollPos = max(_iCurrentScrollPos, 0);
        _iCurrentScrollPos = min(_iCurrentScrollPos, _iScrollRange);
        SendMessageA(_hScrollBar, SBM_SETPOS, _iCurrentScrollPos, TRUE);
        INT iTemp = LOWORD(wParam);

        if (iTemp == SB_LINEDOWN || iTemp == SB_LINEUP ||
            iTemp == SB_THUMBPOSITION || iTemp == SB_PAGEUP || iTemp==SB_PAGEDOWN)
        {
            _writeDataToControls(_iCurrentScrollPos, _iDisplayCount);
        }
        break;
    } 
    return FALSE;
}


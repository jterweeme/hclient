#include "combobox.h"

Combobox::Combobox()
{

}

void Combobox::import(HWND parent, int id)
{
    _parent = parent;
    _id = id;
    _hwnd = GetDlgItem(parent, id);
}

LRESULT Combobox::sendMsgA(UINT msg, WPARAM wp, LPARAM lp) const
{
    return SendMessageA(_hwnd, msg, wp, lp);
}

LRESULT Combobox::addStr(STRSAFE_LPCSTR pszFormat, ...)
{
    CHAR buf[250];
    va_list vl;
    va_start(vl, pszFormat);
    StringCbVPrintfA(buf, 250, pszFormat, vl);
    va_end(vl);
    return sendMsgA(CB_ADDSTRING, 0, LPARAM(buf));
}


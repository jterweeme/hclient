#include "listbox.h"

Listbox::Listbox()
{

}

LRESULT Listbox::addStr(STRSAFE_LPCSTR pszFormat, ...)
{
    CHAR buf[250];
    va_list vl;
    va_start(vl, pszFormat);
    StringCbVPrintfA(buf, 250, pszFormat, vl);
    va_end(vl);
    return sendMsgA(LB_ADDSTRING, 0, LPARAM(buf));
}

LRESULT Listbox::reset()
{
    return sendMsgA(LB_RESETCONTENT, 0, 0);
}


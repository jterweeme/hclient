#include "combobox.h"

Combobox::Combobox()
{

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


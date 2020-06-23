#include "toolbox.h"
#include "hclient.h"

void Toolbox::errorbox(HWND parent, STRSAFE_LPCSTR pszFormat, ...)
{
    CHAR buf[250];
    va_list vl;
    va_start(vl, pszFormat);
    StringCbVPrintfA(buf, 250, pszFormat, vl);
    va_end(vl);
    MessageBoxA(parent, buf, HCLIENT_ERROR, MB_ICONEXCLAMATION);
}


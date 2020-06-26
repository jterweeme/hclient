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

char Toolbox::nibble(BYTE n)
{
    return n < 10 ? '0' + n : 'a' + (n - 10);
}

void Toolbox::hex8(std::ostream &os, BYTE b)
{
    os.put(nibble(b >> 4 & 0xf));
    os.put(nibble(b >> 0 & 0xf));
}

void Toolbox::hex16(std::ostream &os, WORD w)
{
    hex8(os, w >> 8 & 0xff);
    hex8(os, w >> 0 & 0xff);
}

void Toolbox::hexdump(std::ostream &os, const char *buf, int len)
{
    for (int i = 0; i < len; i++)
    {
        hex8(os, buf[i]);
        os.put(' ');
    }
}


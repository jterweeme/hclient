#ifndef TOOLBOX_H
#define TOOLBOX_H

#include <windows.h>
#include <strsafe.h>
#include <iostream>

class Toolbox
{
public:
    static char nibble(BYTE n);
    static void hex8(std::ostream &os, BYTE b);
    static void hex16(std::ostream &os, WORD w);
    static void errorbox(HWND parent, STRSAFE_LPCSTR pszFormat, ...);
    static void hexdump(std::ostream &os, const char *buf, int len);
};

#endif


#ifndef TOOLBOX_H
#define TOOLBOX_H

#include <windows.h>
#include <strsafe.h>

class Toolbox
{
public:
    static void errorbox(HWND parent, STRSAFE_LPCSTR pszFormat, ...);
};

#endif


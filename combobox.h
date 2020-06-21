#ifndef COMBOBOX_H
#define COMBOBOX_H

#include <windows.h>
#include <strsafe.h>

class Combobox
{
private:
    HWND _hwnd;
    HWND _parent;
    int _id;
public:
    Combobox();
    void import(HWND parent, int id);
    LRESULT sendMsgA(UINT msg, WPARAM wp, LPARAM lp) const;
    LRESULT addStr(STRSAFE_LPCSTR pszFormat, ...);
};

#endif


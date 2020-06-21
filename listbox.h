#ifndef LISTBOX_H
#define LISTBOX_H

#include <windows.h>
#include <strsafe.h>

class Listbox
{
private:
    HWND _parent;
    HWND _hwnd;
    int _id;
public:
    Listbox();
    void import(HWND hwnd, int id);
    LRESULT sendMsgA(UINT msg, WPARAM wp, LPARAM lp) const;
    LRESULT addStr(STRSAFE_LPCSTR pszFormat, ...);
};
#endif


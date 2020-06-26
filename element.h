#ifndef ELEMENT_H
#define ELEMENT_H

#include <windows.h>

class Element
{
protected:
    HWND _hwnd;
    HWND _parent;
    int _id;
public:
    void importFromDlg(HWND parent, int id);
    LRESULT sendMsgA(UINT msg, WPARAM wp, LPARAM lp) const;
    BOOL enable(BOOL en = TRUE);
    BOOL disable();
};

#endif


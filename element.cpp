#include "element.h"

void Element::importFromDlg(HWND parent, int id)
{
    _parent = parent;
    _id = id;
    _hwnd = GetDlgItem(parent, id);
}

LRESULT Element::sendMsgA(UINT msg, WPARAM wp, LPARAM lp) const
{
    return SendMessageA(_hwnd, msg, wp, lp);
}

BOOL Element::enable(BOOL en)
{
    return EnableWindow(_hwnd, en);
}

#ifndef LISTBOX_H
#define LISTBOX_H

#include "element.h"
#include <strsafe.h>

class Listbox : public Element
{
private:
public:
    Listbox();
    LRESULT addStr(STRSAFE_LPCSTR pszFormat, ...);
    LRESULT reset();
};
#endif


#ifndef COMBOBOX_H
#define COMBOBOX_H

#include "element.h"
#include <strsafe.h>

class Combobox : public Element
{
private:
public:
    Combobox();
    LRESULT addStr(STRSAFE_LPCSTR pszFormat, ...);
    LRESULT setItemDataA(INT index, LPARAM value);
    LRESULT reset();
};

#endif


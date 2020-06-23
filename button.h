#ifndef BUTTON_H
#define BUTTON_H

#include "element.h"

class Button : public Element
{
public:
    Button();
    BOOL disable();
    BOOL text(LPCSTR s);
};

#endif


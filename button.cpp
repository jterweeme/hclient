#include "button.h"

Button::Button()
{

}

BOOL Button::disable()
{
    return enable(FALSE);
}

BOOL Button::text(LPCSTR s)
{
    return SetWindowTextA(_hwnd, s);
}

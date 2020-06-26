#include "button.h"

Button::Button()
{

}

BOOL Button::text(LPCSTR s)
{
    return SetWindowTextA(_hwnd, s);
}

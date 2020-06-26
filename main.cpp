#include "maindlg.h"
#include "toolbox.h"
#include <iostream>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    (void)lpCmdLine;
    (void)hPrevInstance;
    (void)nCmdShow;
    MainDialog md(hInstance);
    int ret = TRUE;

    try
    {
        md.create();
    }
    catch (const char *e)
    {
        Toolbox().errorbox(NULL, e);
    }
    catch (...)
    {
        ret = FALSE;
    }

    return ret;
}


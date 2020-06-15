#include "maindlg.h"
#include <iostream>

HINSTANCE hGInstance;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    std::cout << "Onzin\r\n";
    std::cout.flush();
    (void)lpCmdLine;
    (void)hPrevInstance;
    (void)nCmdShow;
    hGInstance = hInstance;
    MainDialog md(hInstance);
    int ret = TRUE;

    try
    {
        md.create();
    }
    catch (...)
    {
        ret = FALSE;
    }

    return ret;
}


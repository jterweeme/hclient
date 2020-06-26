#ifndef WRITEDLG_H
#define WRITEDLG_H

#include <windows.h>
#include <iostream>
#include <vector>

#define MAX_LABEL 128
#define MAX_VALUE 128

class WriteData
{
private:
    std::string _label;
    std::string _value;
public:
    void dump(std::ostream &os) const;
    void setLabel(const char *s);
    void setValue(const char *s);
    std::string getLabel() const;
    std::string getValue() const;
};

class WriteDialog
{
private:
    HWND _hDlg;
    size_t _prCount;
    std::vector<WriteData> *_data;
    INT _iDisplayCount;
    INT _iScrollRange;
    INT _iCurrentScrollPos = 0;
    HWND _hScrollBar;
    static const size_t CONTROL_COUNT = 9;
    HINSTANCE _hInst;
    static WriteDialog *_instance;
    static INT_PTR CALLBACK _dlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp);
    INT_PTR _bGetDataDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp);
    void _writeDataToControls(INT iOffset, size_t iCount);
    void _readDataFromControls(INT iOffset, INT iCount);
    INT_PTR _initDialog(HWND hDlg);
public:
    WriteDialog(HINSTANCE hInstance);
    INT_PTR create2(HWND parent, std::vector<WriteData> *data);
};
#endif


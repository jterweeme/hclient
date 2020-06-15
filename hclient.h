#ifndef HCLIENT_H
#define HCLIENT_H

#include "hid.h"
#include "list.h"

#define WM_UNREGISTER_HANDLE    WM_USER+1
#define WM_DISPLAY_READ_DATA    WM_USER+2
#define WM_READ_DONE            WM_USER+3

#define READ_THREAD_TIMEOUT     1000

#define HCLIENT_ERROR           "HClient Error"

#define INFINITE_READS           ((ULONG)-1)

#define INPUT_BUTTON    1
#define INPUT_VALUE     2
#define OUTPUT_BUTTON   3
#define OUTPUT_VALUE    4
#define FEATURE_BUTTON  5
#define FEATURE_VALUE   6
#define HID_CAPS        7
#define DEVICE_ATTRIBUTES 8

#define MAX_LB_ITEMS 200

#define MAX_WRITE_ELEMENTS 100
#define MAX_OUTPUT_ELEMENTS 50

#define CONTROL_COUNT 9
#define MAX_LABEL 128
#define MAX_VALUE 128
#define SMALL_BUFF 128

struct READ_THREAD_CONTEXT
{
    HID_DEVICE *HidDevice;
    HWND DisplayWindow;
    HANDLE DisplayEvent;
    ULONG NumberOfReads;
    BOOL TerminateThread;
};

struct rWriteDataStruct
{
    char szLabel[MAX_LABEL];
    char szValue[MAX_VALUE];
};

struct rGetWriteDataParams
{
    rWriteDataStruct *prItems;
    int iCount;
};

struct DEVICE_LIST_NODE
{
    LIST_ENTRY Hdr;
    HDEVNOTIFY NotificationHandle;
    HID_DEVICE HidDeviceInfo;
    BOOL DeviceOpened;
};
#endif



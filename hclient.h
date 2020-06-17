#ifndef HCLIENT_H
#define HCLIENT_H

#include "hid.h"

#define WM_DISPLAY_READ_DATA    WM_USER+2
#define WM_READ_DONE            WM_USER+3

#define HCLIENT_ERROR           "HClient Error"

#define INPUT_BUTTON    1
#define INPUT_VALUE     2
#define OUTPUT_BUTTON   3
#define OUTPUT_VALUE    4
#define FEATURE_BUTTON  5
#define FEATURE_VALUE   6
#define HID_CAPS        7
#define DEVICE_ATTRIBUTES 8

#define MAX_WRITE_ELEMENTS 100
#define MAX_OUTPUT_ELEMENTS 50

#define CONTROL_COUNT 9
#define MAX_LABEL 128
#define MAX_VALUE 128
#define SMALL_BUFF 128

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
#endif


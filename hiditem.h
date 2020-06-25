#ifndef HIDITEM_H
#define HIDITEM_H

#include <windows.h>
#include <hidsdi.h>

struct HID_DATA
{
    BOOLEAN IsButtonData;
    UCHAR Reserved;
    USAGE UsagePage;
    NTSTATUS Status;
    ULONG ReportID;
    BOOLEAN IsDataSet;

   union {
      struct {
         ULONG UsageMin;       // Variables to track the usage minimum and max
         ULONG UsageMax;       // If equal, then only a single usage
         ULONG MaxUsageLength; // Usages buffer length.
         USAGE *Usages;         // list of usages (buttons ``down'' on the device.

      } ButtonData;
      struct {
         USAGE Usage; // The usage describing this value;
         USHORT Reserved;

         ULONG Value;
         LONG ScaledValue;
      } ValueData;
   };
};

class HidItem
{
protected:
    HIDP_REPORT_TYPE _type;
    USHORT _buflen;
    USHORT _nButtonCaps;
    USHORT _nValueCaps;
public:
    CHAR *_buffer;
    HID_DATA *_data;
private:
    ULONG _dataLen;
    HIDP_BUTTON_CAPS *_buttonCaps;
    HIDP_VALUE_CAPS *_valueCaps;
public:
    HidItem(HIDP_REPORT_TYPE type);
    ~HidItem();
    USHORT bufLen() const;
    ULONG dataLength() const;
    HIDP_BUTTON_CAPS const *buttonCaps() const;
    HIDP_VALUE_CAPS const *valueCaps() const;
    void init(HIDP_CAPS *caps, PHIDP_PREPARSED_DATA ppd);
    void initCaps(PHIDP_PREPARSED_DATA ppd);
    ULONG nValues() const;
    void allocData();
    void initData(PHIDP_PREPARSED_DATA ppd);
    virtual void initType(HIDP_CAPS *caps) = 0;
};

class HidInput : public HidItem
{
public:
    HidInput();
    ~HidInput();
    void initType(HIDP_CAPS *caps) override;
};

class HidOutput : public HidItem
{
public:
    HidOutput();
    ~HidOutput();
    void initType(HIDP_CAPS *caps) override;
};

class HidFeature : public HidItem
{
public:
    HidFeature();
    ~HidFeature();
    void initType(HIDP_CAPS *caps) override;
};

#endif


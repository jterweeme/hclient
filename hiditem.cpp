#include "hiditem.h"

ULONG HidItem::dataLength() const
{
    return _dataLen;
}

HIDP_BUTTON_CAPS const *HidItem::buttonCaps() const
{
    return _buttonCaps;
}

HIDP_VALUE_CAPS const *HidItem::valueCaps() const
{
    return _valueCaps;
}

void HidItem::initCaps(PHIDP_PREPARSED_DATA ppd)
{
    _buttonCaps = new HIDP_BUTTON_CAPS[_nButtonCaps];
    _valueCaps = new HIDP_VALUE_CAPS[_nValueCaps];

    if (_nButtonCaps > 0)
    {
        if (HidP_GetButtonCaps(_type, _buttonCaps, &_nButtonCaps, ppd) != HIDP_STATUS_SUCCESS)
        {
            throw "GetButtonCaps";
        }
    }

    if (_nValueCaps > 0)
    {
        if (HidP_GetValueCaps(_type, _valueCaps, &_nValueCaps, ppd) != HIDP_STATUS_SUCCESS)
        {
            throw "GetValueCaps";
        }
    }
}

ULONG HidItem::nValues() const
{
    ULONG ret = 0;
    for (ULONG i = 0; i < _nValueCaps; i++)
    {
        if (_valueCaps[i].IsRange)
        {
            ret += _valueCaps[i].Range.UsageMax - _valueCaps[i].Range.UsageMin + 1;

            if (_valueCaps[i].Range.UsageMin > _valueCaps[i].Range.UsageMax)
                throw "Range incorrect";
        }
        else
        {
            ret++;
        }
    }

    return ret;
}

USHORT HidItem::bufLen() const
{
    return _buflen;
}

void HidItem::allocData()
{
    _dataLen = _nButtonCaps + nValues();
    _data = new HID_DATA[_dataLen];
}

void HidItem::initData(PHIDP_PREPARSED_DATA ppd)
{
    HID_DATA *data = _data;

    for (ULONG i = 0; i < _nButtonCaps; i++, data++)
    {
        data->IsButtonData = TRUE;
        data->Status = HIDP_STATUS_SUCCESS;
        data->UsagePage = _buttonCaps[i].UsagePage;

        if (_buttonCaps[i].IsRange)
        {
            data->ButtonData.UsageMin = _buttonCaps[i].Range.UsageMin;
            data->ButtonData.UsageMax = _buttonCaps[i].Range.UsageMax;
        }
        else
        {
            data->ButtonData.UsageMin = data->ButtonData.UsageMax = _buttonCaps[i].NotRange.Usage;
        }

        data->ButtonData.MaxUsageLength = HidP_MaxUsageListLength(
                    HidP_Input, _buttonCaps[i].UsagePage, ppd);

        data->ButtonData.Usages = new USAGE[data->ButtonData.MaxUsageLength];
        data->ReportID = _buttonCaps[i].ReportID;
    }

    for (ULONG i = 0; i < _nValueCaps; i++)
    {
        if (_valueCaps[i].IsRange)
        {
            for (USAGE usage = _valueCaps[i].Range.UsageMin;
                 usage <= _valueCaps[i].Range.UsageMax;
                 usage++)
            {
                data->IsButtonData = FALSE;
                data->Status = HIDP_STATUS_SUCCESS;
                data->UsagePage = _valueCaps[i].UsagePage;
                data->ValueData.Usage = usage;
                data->ReportID = _valueCaps[i].ReportID;
                data++;
            }
        }
        else
        {
            data->IsButtonData = FALSE;
            data->Status = HIDP_STATUS_SUCCESS;
            data->UsagePage = _valueCaps[i].UsagePage;
            data->ValueData.Usage = _valueCaps[i].NotRange.Usage;
            data->ReportID = _valueCaps[i].ReportID;
            data++;
        }
    }
}

void HidItem::init(HIDP_CAPS *caps, PHIDP_PREPARSED_DATA ppd)
{
    initType(caps);
    _buffer = new char[_buflen];
    initCaps(ppd);
    allocData();
    initData(ppd);
}

HidItem::HidItem(HIDP_REPORT_TYPE type) : _type(type)
{

}

HidItem::~HidItem()
{

}

HidInput::HidInput() : HidItem(HidP_Input)
{

}

HidInput::~HidInput()
{

}

void HidInput::initType(HIDP_CAPS *caps)
{
    _buflen = caps->InputReportByteLength;
    _nButtonCaps = caps->NumberInputButtonCaps;
    _nValueCaps = caps->NumberInputValueCaps;
}

HidOutput::HidOutput() : HidItem(HidP_Output)
{

}

HidOutput::~HidOutput()
{

}

void HidOutput::initType(HIDP_CAPS *caps)
{
    _buflen = caps->OutputReportByteLength;
    _nButtonCaps = caps->NumberOutputButtonCaps;
    _nValueCaps = caps->NumberOutputValueCaps;
}

HidFeature::HidFeature() : HidItem(HidP_Feature)
{

}

HidFeature::~HidFeature()
{

}

void HidFeature::initType(HIDP_CAPS *caps)
{
    _buflen = caps->FeatureReportByteLength;
    _nButtonCaps = caps->NumberFeatureButtonCaps;
    _nValueCaps = caps->NumberFeatureValueCaps;
}

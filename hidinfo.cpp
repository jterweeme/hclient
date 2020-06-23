#include "hidinfo.h"
#include "hclient.h"

void HidInfo::displayDeviceAttributes(HIDD_ATTRIBUTES *pAttrib, Listbox &lb)
{
    lb.addStr("Vendor ID: 0x%x", pAttrib->VendorID);
    lb.addStr("Product ID: 0x%x", pAttrib->ProductID);
    lb.addStr("Version Number 0x%x", pAttrib->VersionNumber);
}

std::string HidInfo::report(HID_DATA *data)
{
    const int bufsize = 200;
    char buf[bufsize];

    if (data->IsButtonData)
    {
        if (StringCbPrintfA(buf, bufsize, "Usage Page: 0x%x, Usages: ",
                            data->UsagePage) < 0)
        {
            for (UINT j = 0; j < bufsize; j++)
                buf[j] = '\0';

            throw "Error";
        }

        UINT iRemainingBuffer = 0;
        UINT iStringLength = UINT(strlen(buf));
        char *pszWalk = buf + iStringLength;

        if (iStringLength < bufsize)
            iRemainingBuffer = bufsize - iStringLength;

        ULONG i;
        PUSAGE pUsage;
        for (i = 0, pUsage = data->ButtonData.Usages;
             i < data->ButtonData.MaxUsageLength;
             i++, pUsage++)
        {
            if (*pUsage == 0)
                break; // A usage of zero is a non button.

            if (StringCbPrintfA(pszWalk, iRemainingBuffer, " 0x%x", *pUsage) < 0)
                throw "Error";

            iRemainingBuffer -= UINT(strlen(pszWalk));
            pszWalk += strlen(pszWalk);
        }
    }
    else
    {
        StringCbPrintfA(buf, bufsize, "Usage Page: 0x%x, Usage: 0x%x, Scaled: %d Value: %d",
                    data->UsagePage, data->ValueData.Usage, data->ValueData.ScaledValue,
                    data->ValueData.Value);
    }

    return std::string(buf);
}

void HidInfo::displayValueAttributes(HIDP_VALUE_CAPS *pValue, Listbox &lb)
{
    CHAR szTempBuff[SMALL_BUFF];
    lb.addStr("Report ID 0x%x", pValue->ReportID);
    lb.addStr("Usage Page: 0x%x", pValue->UsagePage);
    lb.addStr("Bit size: 0x%x", pValue->BitSize);
    lb.addStr("Report Count: 0x%x", pValue->ReportCount);
    lb.addStr("Unit Exponent: 0x%x", pValue->UnitsExp);
    lb.addStr("Has Null: 0x%x", pValue->HasNull);
    lb.addStr(pValue->IsAlias ? "Alias" : "=====");

    if (pValue->IsRange)
    {
        StringCbPrintfA(szTempBuff, SMALL_BUFF, "Usage Min: 0x%x, Usage Max 0x%x",
                       pValue->Range.UsageMin, pValue->Range.UsageMax);
    }
    else
    {
        StringCbPrintfA(szTempBuff, SMALL_BUFF, "Usage: 0x%x", pValue->NotRange.Usage);
    }

    lb.sendMsgA(LB_ADDSTRING, 0, LPARAM(szTempBuff));

    if (pValue->IsRange)
    {
        StringCbPrintfA(szTempBuff, SMALL_BUFF,
                       "Data Index Min: 0x%x, Data Index Max: 0x%x",
                       pValue->Range.DataIndexMin,
                       pValue->Range.DataIndexMax);
    }
    else
    {
        StringCbPrintfA(szTempBuff, SMALL_BUFF, "DataIndex: 0x%x", pValue->NotRange.DataIndex);
    }

    lb.sendMsgA(LB_ADDSTRING, 0, LPARAM(szTempBuff));

    lb.addStr("Physical Minimum: %d, Physical Maximum: %d",
              pValue->PhysicalMin, pValue->PhysicalMax);

    lb.addStr("Logical Minimum: 0x%x, Logical Maximum: 0x%x",
              pValue->LogicalMin, pValue->LogicalMax);

    if (pValue->IsStringRange)
    {
       StringCbPrintfA(szTempBuff, SMALL_BUFF, "String  Min: 0x%x String Max 0x%x",
                       pValue->Range.StringMin, pValue->Range.StringMax);
    }
    else
    {
        StringCbPrintfA(szTempBuff, SMALL_BUFF, "String Index: 0x%x",
                        pValue->NotRange.StringIndex);
    }
    lb.sendMsgA(LB_ADDSTRING, 0, LPARAM(szTempBuff));

    if (pValue->IsDesignatorRange)
    {
        StringCbPrintfA(szTempBuff, SMALL_BUFF, "Designator Minimum: 0x%x, Max: 0x%x",
                        pValue->Range.DesignatorMin, pValue->Range.DesignatorMax);
    }
    else
    {
        StringCbPrintfA(szTempBuff, SMALL_BUFF, "Designator Index: 0x%x",
                        pValue->NotRange.DesignatorIndex);
    }

    lb.sendMsgA(LB_ADDSTRING, 0, LPARAM(szTempBuff));
    lb.addStr(pValue->IsAbsolute ? "Absolute: Yes" : "Absolute: No");
}

void HidInfo::displayDeviceCaps(HIDP_CAPS *pCaps, Listbox &lb)
{
    lb.sendMsgA(LB_RESETCONTENT, 0, 0);
    lb.addStr("Usage Page: 0x%x", pCaps->UsagePage);
    lb.addStr("Usage: 0x%x", pCaps->Usage);
    lb.addStr("Input report byte length: %d", pCaps->InputReportByteLength);
    lb.addStr("Output report byte length: %d", pCaps->OutputReportByteLength);
    lb.addStr("Feature report byte length: %d", pCaps->FeatureReportByteLength);
    lb.addStr("Number of collection nodes %d: ", pCaps->NumberLinkCollectionNodes);
}

void HidInfo::displayInputButtons(HidDevice *pDevice, Listbox &lb)
{
    lb.sendMsgA(LB_RESETCONTENT, 0, LPARAM(0));
    HIDP_BUTTON_CAPS *pButtonCaps = pDevice->getp()->InputButtonCaps;
    const USHORT nCaps = pDevice->getp()->Caps.NumberInputButtonCaps;

    for (INT iLoop = 0; iLoop < nCaps; iLoop++)
    {
        INT iIndex = INT(lb.addStr("Input button cap # %d", iLoop));

        if (iIndex != -1)
            lb.sendMsgA(LB_SETITEMDATA, iIndex, LPARAM(pButtonCaps));

        pButtonCaps++;
    }

    lb.sendMsgA(LB_SETCURSEL, 0, 0);
}

void HidInfo::displayButtonAttributes(HIDP_BUTTON_CAPS *pButton, Listbox &lb)
{
    HidButtonCaps caps(*pButton);
    CHAR szTempBuff[SMALL_BUFF];
    lb.addStr("Report ID: 0x%x", pButton->ReportID);
    lb.addStr("Usage Page: 0x%x", pButton->UsagePage);
    lb.addStr("Alias: %s", pButton->IsAlias ? "TRUE" : "FALSE");
    lb.addStr("Link Collection: %hu", pButton->LinkCollection);
    lb.addStr("Link Usage Page: 0x%x", pButton->LinkUsagePage);
    lb.addStr("Link Usage: 0x%x", pButton->LinkUsage);

    if (caps.isRange())
    {
        lb.addStr("Usage Min: 0x%x, Usage Max: 0x%x", caps.usageMin(), caps.usageMax());
    }
    else
    {
        lb.addStr("Usage: 0x%x", pButton->NotRange.Usage);
    }

    if (pButton->IsRange)
    {
        StringCbPrintfA(szTempBuff, SMALL_BUFF,
                 "Data Index Min: 0x%x, Data Index Max: 0x%x",
                 pButton->Range.DataIndexMin, pButton->Range.DataIndexMax);
    }
    else
    {
         StringCbPrintfA(szTempBuff, SMALL_BUFF, "DataIndex: 0x%x", pButton->NotRange.DataIndex);
    }

    lb.addStr(szTempBuff);

    if (pButton->IsStringRange)
    {
        StringCbPrintfA(szTempBuff, SMALL_BUFF, "String Min: 0x%x, String Max: 0x%x",
                       pButton->Range.StringMin, pButton->Range.StringMax);
    }
    else
    {
        StringCbPrintfA(szTempBuff, SMALL_BUFF, "String Index: 0x%x",
                        pButton->NotRange.StringIndex);
    }

    lb.addStr(szTempBuff);

    if (pButton->IsDesignatorRange)
    {
        StringCbPrintfA(szTempBuff, SMALL_BUFF,
                       "Designator Min: 0x%x, Designator Max: 0x%x",
                       pButton->Range.DesignatorMin,
                       pButton->Range.DesignatorMax);
    }
    else
    {
        StringCbPrintfA(szTempBuff, SMALL_BUFF, "Designator Index: 0x%x",
                       pButton->NotRange.DesignatorIndex);
    }

    lb.addStr(szTempBuff);
    lb.addStr(pButton->IsAbsolute ? "Absolute: Yes" : "Absolute: No");
}

void HidInfo::displayInputValues(HidDevice *pDevice, Listbox &lb)
{
    CHAR szTempBuff[SMALL_BUFF];
    lb.sendMsgA(LB_RESETCONTENT, 0, 0);
    HIDP_VALUE_CAPS *pValueCaps = pDevice->getp()->InputValueCaps;

    for (INT iLoop = 0; iLoop < pDevice->getp()->Caps.NumberInputValueCaps; iLoop++)
    {
        StringCbPrintfA(szTempBuff, SMALL_BUFF, "Input value cap # %d", iLoop);
        INT iIndex = INT(lb.sendMsgA(LB_ADDSTRING, 0, LPARAM(szTempBuff)));

        if (iIndex != -1)
            lb.sendMsgA(LB_SETITEMDATA, iIndex, LPARAM(pValueCaps));

        pValueCaps++;
    }

    lb.sendMsgA(LB_SETCURSEL, 0, 0);
}

void HidInfo::displayOutputButtons(HidDevice *pDevice, Listbox &lb)
{
    lb.sendMsgA(LB_RESETCONTENT, 0, LPARAM(0));
    HIDP_BUTTON_CAPS *pButtonCaps = pDevice->getp()->OutputButtonCaps;

    for (INT iLoop = 0; iLoop < pDevice->getp()->Caps.NumberOutputButtonCaps; iLoop++)
    {
        INT iIndex = INT(lb.addStr("Output button cap # %d", iLoop));

        if (iIndex != -1)
            lb.sendMsgA(LB_SETITEMDATA, iIndex, LPARAM(pButtonCaps));

        pButtonCaps++;
    }

    lb.sendMsgA(LB_SETCURSEL, 0, 0);
}

void HidInfo::displayOutputValues(HidDevice *pDevice, Listbox &lb)
{
    lb.sendMsgA(LB_RESETCONTENT, 0, 0);
    HIDP_VALUE_CAPS *pValueCaps = pDevice->getp()->OutputValueCaps;

    for (INT iLoop = 0; iLoop < pDevice->getp()->Caps.NumberOutputValueCaps; iLoop++)
    {
        INT iIndex = INT(lb.addStr("Output value cap # %d", iLoop));

        if (iIndex != -1)
            lb.sendMsgA(LB_SETITEMDATA, iIndex, LPARAM(pValueCaps));

        pValueCaps++;
    }

    lb.sendMsgA(LB_SETCURSEL, 0, 0);
}

void HidInfo::displayFeatureButtons(HidDevice *pDevice, Listbox &lb)
{
    CHAR szTempBuff[SMALL_BUFF];
    lb.sendMsgA(LB_RESETCONTENT, 0, 0);
    HIDP_BUTTON_CAPS *pButtonCaps = pDevice->getp()->FeatureButtonCaps;

    USHORT nCaps = pDevice->getp()->Caps.NumberFeatureButtonCaps;
    for (USHORT iLoop = 0; iLoop < nCaps; iLoop++)
    {
        StringCbPrintfA(szTempBuff, SMALL_BUFF, "Feature button cap # %d", iLoop);
        INT iIndex = INT(lb.sendMsgA(LB_ADDSTRING, 0, LPARAM(szTempBuff)));

        if (iIndex != -1)
            lb.sendMsgA(LB_SETITEMDATA, iIndex, LPARAM(pButtonCaps));

        pButtonCaps++;
    }
    lb.sendMsgA(LB_SETCURSEL, 0, 0);
}

void HidInfo::displayFeatureValues(HidDevice *pDevice, Listbox &lb)
{
    CHAR szTempBuff[SMALL_BUFF];
    lb.sendMsgA(LB_RESETCONTENT, 0, 0);
    HIDP_VALUE_CAPS *pValueCaps = pDevice->getp()->FeatureValueCaps;

    for (INT iLoop = 0; iLoop < pDevice->getp()->Caps.NumberFeatureValueCaps; iLoop++)
    {
        StringCbPrintfA(szTempBuff, SMALL_BUFF, "Feature value cap # %d", iLoop);
        INT iIndex = INT(lb.sendMsgA(LB_ADDSTRING, 0, LPARAM(szTempBuff)));

        if (iIndex != -1)
            lb.sendMsgA(LB_SETITEMDATA, iIndex, LPARAM(pValueCaps));

        pValueCaps++;
    }

    lb.sendMsgA(LB_SETCURSEL, 0, 0);
}


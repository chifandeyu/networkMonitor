#include "common.h"

std::wstring GUIDToString(const GUID &guid)
{
    OLECHAR guidString[40] = { 0 };
    ::StringFromGUID2(guid, guidString, sizeof(guidString));
    return guidString;
}

std::string UnicodeToUTF8(const std::wstring &wstr)
{
    std::string ret;
    try
    {
        std::wstring_convert< std::codecvt_utf8<wchar_t> > wcv;
        ret = wcv.to_bytes(wstr);
    }
    catch (const std::exception & e)
    {
        std::cerr << e.what() << std::endl;
    }
    return ret;
}

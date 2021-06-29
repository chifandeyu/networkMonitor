#ifndef COMMON_H
#define COMMON_H
#include <string>
#include <Windows.h>
#include <codecvt>        // std::codecvt_utf8
#include <iostream>
#include <memory>
#include <thread>
#include <vector>
#include <algorithm>
#include <locale>
#include <QMetaType>
#include <netlistmgr.h>
#include <atlbase.h>
#include <objbase.h>
#include <wtypes.h>

std::wstring GUIDToString(const GUID & guid);

std::string UnicodeToUTF8(const std::wstring & wstr);

enum NetworkType
{
    NotNetwork,
    Ethernet,
    Wlan,
};
Q_DECLARE_METATYPE(NetworkType)

enum WiFiQuality
{
    Weak,             // (-INF, -70dBm)    弱的
    Fair,             // [-70dBm, -60dBm)  一般
    Good,             // [-60dBm, -50dBm)  良好
    Excellent         // [-50dBm, +INF)    优秀
};
Q_DECLARE_METATYPE(WiFiQuality)

struct ConnectionInfo
{
    std::wstring guid;
    NetworkType type;
};

#endif // COMMON_H

#ifndef NETWORKMONITOR_H
#define NETWORKMONITOR_H

#include <QObject>
#include "common.h"
#include <wlanapi.h>
#include <IPHlpApi.h>
#include "networkevent.h"
#include <QMutex>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "wlanapi.lib")
#pragma comment(lib, "Iphlpapi.lib")

class NetworkMonitor : public QObject
{
    Q_OBJECT
    friend void OnNotificationCallback(PWLAN_NOTIFICATION_DATA Data, PVOID context);
public:
    explicit NetworkMonitor(QObject *parent = nullptr);
    ~NetworkMonitor();

    std::vector<ConnectionInfo> GetNetworkConnections();
    NetworkType GetNetAdpaterType(const std::wstring &guid);
    void OnNetworkStatusChange();
    WiFiQuality GetWiFiSignalQuality(const std::wstring &guid);
    void OnWiFiQualityChange(const GUID & guid);

    void ShowNetworkStatus();
    static NetworkMonitor *getNetworkMonitor();
    static void clearNetworkMonitor();

signals:
    void sigNetworkStatus(NetworkType type, WiFiQuality quality);
private:
    NetWorkEvent* _networkEvent = nullptr;

    DWORD _cookie;
    INetworkListManager*       _pNLM = nullptr;
    IConnectionPointContainer* _pCpc = nullptr;
    IConnectionPoint*          _pConnectionPoint = nullptr;

    HANDLE _wlanHandle = NULL;
    static NetworkMonitor *m_pInstance;
};

#endif // NETWORKMONITOR_H

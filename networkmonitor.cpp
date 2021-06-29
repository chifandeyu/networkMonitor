#include "networkmonitor.h"
#include <QDebug>

void OnNotificationCallback(PWLAN_NOTIFICATION_DATA Data, PVOID context)
{
    if (Data != NULL &&
        Data->NotificationSource == WLAN_NOTIFICATION_SOURCE_MSM &&
        Data->NotificationCode == wlan_notification_msm_signal_quality_change)
    {
        WLAN_SIGNAL_QUALITY Qality = (WLAN_SIGNAL_QUALITY)Data->pData;
        std::cout << "WiFi OnNotification Qality : " << Qality << std::endl;
        NetworkMonitor::getNetworkMonitor()->OnWiFiQualityChange(Data->InterfaceGuid);
    }
}

QMutex g_mutex;
NetworkMonitor* NetworkMonitor::m_pInstance = nullptr;

NetworkMonitor::NetworkMonitor(QObject *parent/* = nullptr*/)
    : QObject(parent)
{
    CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE);
    qRegisterMetaType<NetworkType>("NetworkType");
    qRegisterMetaType<WiFiQuality>("WiFiQuality");
    HRESULT hr = CoCreateInstance(CLSID_NetworkListManager, NULL, CLSCTX_ALL, IID_INetworkListManager, (LPVOID *)&_pNLM);
    hr = _pNLM->QueryInterface(IID_IConnectionPointContainer, (void **)&_pCpc);
    hr = _pCpc->FindConnectionPoint(IID_INetworkConnectionEvents, &_pConnectionPoint);

    _networkEvent =  new NetWorkEvent(std::bind(&NetworkMonitor::OnNetworkStatusChange, this));
    IUnknown* ptr = static_cast<IUnknown*>(_networkEvent);
    hr = _pConnectionPoint->Advise(ptr, &_cookie);
}

NetworkMonitor::~NetworkMonitor()
{
    if (_pConnectionPoint)
    {
        _pConnectionPoint->Unadvise(_cookie);
    }
    if(_networkEvent){
        delete _networkEvent;
        _networkEvent = nullptr;
    }
    WlanCloseHandle(_wlanHandle, NULL);

    CoUninitialize();
}

std::vector<ConnectionInfo> NetworkMonitor::GetNetworkConnections()
{
    std::vector<ConnectionInfo> result;
    CoInitialize(NULL);
    IEnumNetworkConnections* enumConnectons;

    if (FAILED(_pNLM->GetNetworkConnections(&enumConnectons)))
    {
        std::cerr << "GetNetworkConnections error : " << GetLastError() << std::endl;
        CoUninitialize();
        return result;
    }

    if (enumConnectons)
    {
        ULONG lFetch;
        INetworkConnection *connection = nullptr;
        while (SUCCEEDED(enumConnectons->Next(1, &connection, &lFetch)) && nullptr != connection)
        {
            VARIANT_BOOL isConnectInternet = VARIANT_FALSE;
            connection->get_IsConnectedToInternet(&isConnectInternet);
            if (isConnectInternet == VARIANT_FALSE)
            {
                continue;
            }
            ConnectionInfo item;
            GUID guid;

            connection->GetAdapterId(&guid);
            item.guid = GUIDToString(guid);

            result.push_back(item);
        }
        if (connection)
        {
            connection->Release();
        }
    }

    for (auto it = result.begin(); it != result.end(); ++it)
    {
        it->type = GetNetAdpaterType(it->guid);
    }
    std::partition(result.begin(), std::partition(result.begin(), result.end(), [](const ConnectionInfo & info)
    {
        return info.type != NetworkType::NotNetwork;
    }), [](const ConnectionInfo & info)
    {
        return info.type == NetworkType::Ethernet;
    });
    for (auto it = result.begin(); it != result.end(); ++it)
    {
        std::wcout << "connect network guid : " << it->guid << " | type : " << it->type << std::endl;
    }
    CoUninitialize();
    return result;
}

NetworkType NetworkMonitor::GetNetAdpaterType(const std::wstring &guid)
{
    unsigned long unSize = sizeof(IP_ADAPTER_INFO);
    std::unique_ptr<uint8_t[]> data = std::make_unique<uint8_t[]>(unSize);
    //bool find = false;
    unsigned long unResult = GetAdaptersInfo(reinterpret_cast<PIP_ADAPTER_INFO>(data.get()), &unSize);

    if (ERROR_BUFFER_OVERFLOW == unResult)
    {
        data = std::make_unique<uint8_t[]>(unSize);
        unResult = GetAdaptersInfo(reinterpret_cast<PIP_ADAPTER_INFO>(data.get()), &unSize);
    }
    if (ERROR_SUCCESS == unResult)
    {
        PIP_ADAPTER_INFO pIpAdapterInfo = reinterpret_cast<PIP_ADAPTER_INFO>(data.get());

        while (pIpAdapterInfo)
        {
            if (UnicodeToUTF8(guid) == pIpAdapterInfo->AdapterName)
            {
                return MIB_IF_TYPE_ETHERNET == pIpAdapterInfo->Type ? NetworkType::Ethernet :
                    IF_TYPE_IEEE80211 == pIpAdapterInfo->Type ? NetworkType::Wlan : NetworkType::NotNetwork;
            }
            pIpAdapterInfo = pIpAdapterInfo->Next;
        }
    }

    return NetworkType::NotNetwork;
}

void NetworkMonitor::OnNetworkStatusChange()
{
    ShowNetworkStatus();
}

WiFiQuality NetworkMonitor::GetWiFiSignalQuality(const std::wstring &guid)
{
    WiFiQuality result = WiFiQuality::Weak;
    DWORD dwMaxClient = 2;
    DWORD dwCurVersion = 0;
    DWORD dwResult = 0;

    unsigned int i, j;

    PWLAN_INTERFACE_INFO_LIST pIfList = NULL;
    PWLAN_INTERFACE_INFO pIfInfo = NULL;

    PWLAN_AVAILABLE_NETWORK_LIST pBssList = NULL;
    PWLAN_AVAILABLE_NETWORK pBssEntry = NULL;

    int iRSSI = 0;

    if (_wlanHandle == NULL)
    {
        dwResult = WlanOpenHandle(dwMaxClient, NULL, &dwCurVersion, &_wlanHandle);
        if (dwResult != ERROR_SUCCESS)
        {
            std::cerr << "WlanOpenHandle failed with error: " << dwResult << std::endl;
            return result;
        }
        dwResult = WlanRegisterNotification(_wlanHandle, WLAN_NOTIFICATION_SOURCE_ALL, TRUE, WLAN_NOTIFICATION_CALLBACK(OnNotificationCallback), NULL, NULL, NULL);
        if (dwResult != ERROR_SUCCESS)
        {
            std::cerr << "WlanRegisterNotification failed with error: " << dwResult << std::endl;
            return result;
        }
    }

    dwResult = WlanEnumInterfaces(_wlanHandle, NULL, &pIfList);
    if (dwResult != ERROR_SUCCESS)
    {
        std::cerr << "WlanEnumInterfaces failed with error: " << dwResult << std::endl;
        return result;
    }
    for (i = 0; i < (int)pIfList->dwNumberOfItems; i++)
    {
        pIfInfo = (WLAN_INTERFACE_INFO *)&pIfList->InterfaceInfo[i];
        if (guid != GUIDToString(pIfInfo->InterfaceGuid) || pIfInfo->isState != wlan_interface_state_connected)
        {
            continue;
        }

        dwResult = WlanGetAvailableNetworkList(_wlanHandle,
                                               &pIfInfo->InterfaceGuid,
                                               0,
                                               NULL,
                                               &pBssList);

        if (dwResult != ERROR_SUCCESS)
        {
            std::cerr << "WlanGetAvailableNetworkList failed with error:" << dwResult << std::endl;
            return result;
        }
        for (j = 0; j < pBssList->dwNumberOfItems; j++)
        {
            pBssEntry = (WLAN_AVAILABLE_NETWORK *)& pBssList->Network[j];

            if (pBssEntry->bNetworkConnectable && (pBssEntry->dwFlags & WLAN_AVAILABLE_NETWORK_CONNECTED))
            {
                if (pBssEntry->wlanSignalQuality == 0)
                    iRSSI = -100;
                else if (pBssEntry->wlanSignalQuality == 100)
                    iRSSI = -50;
                else
                    iRSSI = -100 + (pBssEntry->wlanSignalQuality / 2);

                std::cout << "Signal Quality:\t " << pBssEntry->wlanSignalQuality << " (RSSI: " << iRSSI << " dBm)" << std::endl;
                std::cout << "wifi name:\t" << pBssEntry->strProfileName ;
                result = iRSSI < -70 ? WiFiQuality::Weak :
                    iRSSI < -60 ? WiFiQuality::Fair :
                    iRSSI < -50 ? WiFiQuality::Good : WiFiQuality::Excellent;
            }
        }
    }
    if (pBssList != NULL)
    {
        WlanFreeMemory(pBssList);
        pBssList = NULL;
    }

    if (pIfList != NULL)
    {
        WlanFreeMemory(pIfList);
        pIfList = NULL;
    }
    return result;
}

void NetworkMonitor::OnWiFiQualityChange(const GUID &guid)
{
    auto nowQuality = GetWiFiSignalQuality(GUIDToString(guid));
    std::cout << "WiFi signal quality now : " << nowQuality << std::endl;
    ShowNetworkStatus();
}

void NetworkMonitor::ShowNetworkStatus()
{
    NetworkType type = NotNetwork;
    WiFiQuality quality = WiFiQuality::Weak;
    auto connections = GetNetworkConnections();
    if (connections.empty())
    {
        type = NetworkType::NotNetwork;
    }
    else
    {
        type = connections.front().type;
        if (type == NetworkType::Wlan)
        {
            quality = GetWiFiSignalQuality(connections.front().guid);
        }
    }

    std::cout << "====== Notify ======" << std::endl;
    qDebug() << "* Type : " << (type == NetworkType::NotNetwork ? "NetworkError" : type == NetworkType::Ethernet ? "Ethernet" : "WiFi");
    if (type == NetworkType::Wlan)
    {
        std::cout << "* Signal : " << quality + 1 << std::endl;
    }
    emit sigNetworkStatus(type, quality);
    std::cout << std::endl;
}

void NetworkMonitor::clearNetworkMonitor()
{
    if (m_pInstance) {
        delete m_pInstance;
        m_pInstance = nullptr;
    }
}

NetworkMonitor *NetworkMonitor::getNetworkMonitor()
{
    if(!m_pInstance){
        QMutexLocker locker(&g_mutex);
        if(!m_pInstance){
           m_pInstance = new NetworkMonitor();
        }
    }
    return m_pInstance;
}

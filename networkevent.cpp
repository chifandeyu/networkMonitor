#include "networkevent.h"

NetWorkEvent::NetWorkEvent(const std::function<void()> & cb)
    : _callback(cb)
{

}

HRESULT NetWorkEvent::NetworkConnectionConnectivityChanged(GUID connectionId, NLM_CONNECTIVITY newConnectivity)
{
    std::wcout << GUIDToString(connectionId) << " | NUL_CONNECTIVITY : " << newConnectivity << std::endl;
    if (_callback)
    {
        _callback();
    }
    return S_OK;
}

HRESULT NetWorkEvent::NetworkConnectionPropertyChanged(GUID /*connectionId*/, NLM_CONNECTION_PROPERTY_CHANGE /*flags*/)
{
    return S_OK;
}

HRESULT NetWorkEvent::QueryInterface(const IID &refIID, void **pIFace)
{
    HRESULT hr = S_OK;
    *pIFace = NULL;
    if (IsEqualIID(refIID, IID_IUnknown))
    {
        *pIFace = (IUnknown *)this;
        ((IUnknown *)*pIFace)->AddRef();
    }
    else if (IsEqualIID(refIID, IID_INetworkConnectionEvents))
    {
        *pIFace = (INetworkConnectionEvents *)this;
        ((IUnknown *)*pIFace)->AddRef();
    }
    else
    {
        hr = E_NOINTERFACE;
    }

    return hr;
}

ULONG NetWorkEvent::AddRef()
{
    return (ULONG)InterlockedIncrement(&_ref);
}

ULONG NetWorkEvent::Release()
{
    LONG Result = InterlockedDecrement(&_ref);
    if (Result == 0)
        delete this;
    return (ULONG)Result;
}

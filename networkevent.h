#ifndef NETWORKEVENT_H
#define NETWORKEVENT_H

#include "common.h"
#include <netlistmgr.h>
#include <functional>

class NetWorkEvent : public INetworkConnectionEvents
{
public:
    explicit NetWorkEvent(const std::function<void()> & cb);
    virtual HRESULT STDMETHODCALLTYPE NetworkConnectionConnectivityChanged(
        /* [in] */ GUID connectionId,
        /* [in] */ NLM_CONNECTIVITY newConnectivity);

    virtual HRESULT STDMETHODCALLTYPE NetworkConnectionPropertyChanged(
        /* [in] */ GUID connectionId,
        /* [in] */ NLM_CONNECTION_PROPERTY_CHANGE flags);

    STDMETHODIMP QueryInterface(REFIID refIID, void **pIFace);
    virtual ULONG __stdcall AddRef(void);
    virtual ULONG __stdcall Release(void);

private:
    LONG _ref;
    std::function<void()> _callback = nullptr;
};

#endif // NETWORKEVENT_H

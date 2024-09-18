#include "DXDebugLayer.h"

bool DXDebugLayer::Init()
{
#ifdef _DEBUG
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&m_id3d12Debug)))) {
        m_id3d12Debug->EnableDebugLayer();
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&m_idxgiDebug)))) {
            m_idxgiDebug->EnableLeakTrackingForThread();
            return true;
        }
    }
#endif // _DEBUG

    return false;
}

void DXDebugLayer::Shutdown()
{
#ifdef _DEBUG

    if (m_idxgiDebug) {
        OutputDebugStringW(L"DXGI Reports living device objects:\n");
        m_idxgiDebug->ReportLiveObjects(
            DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL)
        );
    }

    m_id3d12Debug.Release();
    m_idxgiDebug.Release();
#endif //_DEBUG
}

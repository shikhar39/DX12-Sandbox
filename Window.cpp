#include "Window.h"

bool DXWindow::Init()
{

    // ############### Window Class ###############
    WNDCLASSEXW wcex{};
    wcex.cbSize = sizeof(wcex);
    wcex.style = CS_OWNDC;
    wcex.lpfnWndProc = &DXWindow::OnWindowMessage;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = GetModuleHandleW(nullptr);
    wcex.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wcex.hbrBackground = nullptr;
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = L"D3D12ExWndCls";
    wcex.hIconSm = LoadIconW(nullptr, IDI_APPLICATION);

    m_wndClass = RegisterClassExW(&wcex);
    // if this fails, return False
    if (m_wndClass == 0) {
        return false;
    }

    // ############### Window ###############
    m_window = CreateWindowExW(
        WS_EX_OVERLAPPEDWINDOW | WS_EX_APPWINDOW,
        LPCWSTR(m_wndClass),
        L"D3D12 Window",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        100, 100,
        m_width, m_height,
        nullptr,
        nullptr,
        wcex.hInstance,
        nullptr
    );
    
    if (m_window == nullptr) {
        return false; 
    }

    // ############### Swap Chain Description ###############
    DXGI_SWAP_CHAIN_DESC1 swd{};
    DXGI_SWAP_CHAIN_FULLSCREEN_DESC sfd{};

    swd.Width = m_width;
    swd.Height = m_height;
    swd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swd.Stereo = false;
    swd.SampleDesc.Count = 1;
    swd.SampleDesc.Quality = 0;
    swd.BufferUsage = DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swd.BufferCount = GetFrameCount();
    swd.Scaling = DXGI_SCALING_STRETCH;
    swd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swd.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
    swd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

    sfd.Windowed = true;



    // ############### Swap Chain ###############
    // Manages buffers and stuff. Ex: swapping front and back buffers etc. 

    auto& factory = DXContext::Get().GetFactory();
    ComPointer<IDXGISwapChain1> sc1;
    factory->CreateSwapChainForHwnd(
        DXContext::Get().GetCommandQueue(),
        m_window,
        &swd,
        &sfd,
        nullptr,
        &sc1
    );
    if (!sc1.QueryInterface(m_swapChain)) {
        return false;
    }
    
    // RTV Descriptor Heap
    D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc{};
    descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    descHeapDesc.NumDescriptors = FrameCount;
    descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    descHeapDesc.NodeMask = 0;
    if (FAILED(DXContext::Get().GetDevice()->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&m_rtvDescHeap)))) {
        return false;
    }
    
    // Create Handles to view 
    auto firstHandle = m_rtvDescHeap->GetCPUDescriptorHandleForHeapStart();
    auto handleIncrement = DXContext::Get().GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    for (size_t i = 0; i < FrameCount; i++) {
        m_rtvHandles[i] = firstHandle;
        m_rtvHandles[i].ptr += handleIncrement * i;
    }

    //Get Buffers
    if (!GetBuffers()) {
        return false;
    }
    return true;

}

void DXWindow::Update()
{
    // Handle messages coming into the window ex: minimize, resize, close, and tons of other stuff
    MSG msg;
    while(PeekMessageW(&msg, m_window, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

void DXWindow::Preset()
{
    // I think this is the command that actually swaps the front and back buffers. 
    m_swapChain->Present(1, 0);
    
}

void DXWindow::Shutdown()
{
    ReleaseBuffers();
    
    m_rtvDescHeap.Release();

    m_swapChain.Release();
    
    if (m_window) {
        DestroyWindow(m_window);
    }
    if (m_wndClass) {
        UnregisterClassW(LPCWSTR(m_wndClass), GetModuleHandleW(nullptr));
    }
}

void DXWindow::Resize()
{
    // Need to release reference to buffers on the GPU before new, different size buffer can be allocated and referenced. 
    ReleaseBuffers();

    // Resizng th buffer based on new window dimensions
    RECT cr;
    if (GetClientRect(m_window, &cr)) {
        
        m_width = cr.right - cr.left;
        m_height = cr.bottom - cr.top;

        m_swapChain->ResizeBuffers(GetFrameCount(), m_width, m_height, DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH |DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING);
        m_shouldResize = false;
    }

    // Allocate the new buffers to m_buffers pointer
    GetBuffers();
}

void DXWindow::SetFullscreen(bool enabled) {
    
    // Update window style
    DWORD style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
    DWORD exStyle = WS_EX_OVERLAPPEDWINDOW | WS_EX_APPWINDOW;
    if (enabled) {
        style = WS_POPUP | WS_VISIBLE;
        exStyle = WS_EX_APPWINDOW;
    }
    SetWindowLongW(m_window, GWL_STYLE, style);
    SetWindowLongW(m_window, GWL_EXSTYLE, exStyle);

    // Adjust window size

    if (enabled) {
        HMONITOR monitor = MonitorFromWindow(m_window, MONITOR_DEFAULTTONEAREST);
        MONITORINFO monitorInfo{};
        monitorInfo.cbSize = sizeof(monitorInfo);
        if (GetMonitorInfoW(monitor, &monitorInfo)) {
            SetWindowPos(m_window, nullptr,
                monitorInfo.rcMonitor.left,
                monitorInfo.rcMonitor.top,
                monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
                monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
                SWP_NOZORDER
            );
        }
    }
    else {
        ShowWindow(m_window, SW_MAXIMIZE);
    }

    m_isFullscreen = enabled;
}

void DXWindow::BeginFrame(ID3D12GraphicsCommandList6* cmdList)
{
    m_currBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
    D3D12_RESOURCE_BARRIER barr;

    barr.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barr.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barr.Transition.pResource = m_buffers[m_currBufferIndex];
    barr.Transition.Subresource = 0;
    barr.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barr.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

    cmdList->ResourceBarrier(1, &barr);
    float clearColor[] = {0.4f, .4f, 0.8f, 1.f };
    cmdList->ClearRenderTargetView(m_rtvHandles[m_currBufferIndex], clearColor, 0, nullptr);

    cmdList->OMSetRenderTargets(1, &m_rtvHandles[m_currBufferIndex], false, nullptr);
}

void DXWindow::EndFrame(ID3D12GraphicsCommandList6* cmdList)
{
    D3D12_RESOURCE_BARRIER barr;

    barr.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barr.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barr.Transition.pResource = m_buffers[m_currBufferIndex];
    barr.Transition.Subresource = 0;
    barr.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barr.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

    cmdList->ResourceBarrier(1, &barr);
}


bool DXWindow::GetBuffers()
{
    // Allocate buffers to m_buffers
    for (size_t i = 0; i < FrameCount; i++) {
        if (FAILED(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_buffers[i])))) {
            return false;
        }

        D3D12_RENDER_TARGET_VIEW_DESC rtv{};
        rtv.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        rtv.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        rtv.Texture2D.MipSlice = 0;
        rtv.Texture2D.PlaneSlice = 0; 

        DXContext::Get().GetDevice()->CreateRenderTargetView(m_buffers[i], &rtv, m_rtvHandles[i]);

    }
    return true;
}

void DXWindow::ReleaseBuffers()
{
    // De-allocate buffers from m_buffers
    for (size_t i = 0; i < FrameCount; i++) {
        m_buffers[i].Release();
    }
}


// Callback function that is given to Window class on top
LRESULT DXWindow::OnWindowMessage(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
{

    
    switch (msg)
    {
        // Fullscreen with F11 key
        case WM_KEYDOWN:
            if (wParam == VK_F11) {
                Get().SetFullscreen(!Get().IsFullscreen());
            }
            break;
        // Handle resizing on the resize event
        case WM_SIZE:
            if (lParam && (HIWORD(lParam) != Get().m_height || LOWORD(lParam) != Get().m_width)  ) {
                Get().m_shouldResize = true;
            }
            break;
        // Initiate shutdown on the window close event
        case WM_CLOSE:
            Get().m_shouldClose = true;
            return 0;
            break;
    }
    return DefWindowProcW(wnd, msg, wParam, lParam);
}

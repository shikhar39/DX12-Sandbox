#pragma once
#include "../Support/WinInclude.h"
#include "../Support/ComPointer.h"

class DXDebugLayer
{
public:
	bool Init();
	void Shutdown();

private:
#ifdef _DEBUG
	ComPointer<ID3D12Debug6> m_id3d12Debug;
	ComPointer<IDXGIDebug1> m_idxgiDebug;
#endif


//Singleton
public:
	DXDebugLayer(const DXDebugLayer&) = delete;
	DXDebugLayer& operator=(const DXDebugLayer&) = delete;
	inline static DXDebugLayer& Get() {
		static DXDebugLayer instance;
		return instance; 
	}
private:
	DXDebugLayer() = default;

};


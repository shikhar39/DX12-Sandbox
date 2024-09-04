#include <iostream>
#include "WinInclude.h"
#include "ComPointer.h"
#include "Window.h"

#include "DXDebugLayer.h"

#include "DXContext.h"

void main() {
	DXDebugLayer::Get().Init();

	if (DXContext::Get().Init() && DXWindow::Get().Init()) {
		

		D3D12_HEAP_PROPERTIES hpUpload{};
		hpUpload.Type = D3D12_HEAP_TYPE_UPLOAD;
		hpUpload.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		hpUpload.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		hpUpload.CreationNodeMask = 0;
		hpUpload.VisibleNodeMask = 0;

		D3D12_HEAP_PROPERTIES hpDefault{};
		hpDefault.Type = D3D12_HEAP_TYPE_DEFAULT;
		hpDefault.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		hpDefault.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		hpDefault.CreationNodeMask = 0;
		hpDefault.VisibleNodeMask = 0;

		// Vertex Data
		struct Vertex {
			float x, y;
		};
		Vertex vertices[] = {
			// Triangle 1
			{-1.f, -1.f},
			{ 0.f, 1.f},
			{1.f, -1.f}
		};
		D3D12_INPUT_ELEMENT_DESC vertexLayout[] = {
			{	"Position",	0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};


		D3D12_RESOURCE_DESC rd{};
		rd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		rd.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
		rd.Width = 1024;
		rd.Height = 1;
		rd.DepthOrArraySize = 1;
		rd.MipLevels = 1;
		rd.Format = DXGI_FORMAT_UNKNOWN;
		rd.SampleDesc.Count = 1;
		rd.SampleDesc.Quality = 0;
		rd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		rd.Flags = D3D12_RESOURCE_FLAG_NONE;

		ComPointer<ID3D12Resource2> uploadBuffer, vertexBuffer;
		DXContext::Get().GetDevice()->CreateCommittedResource(&hpUpload, D3D12_HEAP_FLAG_NONE, &rd, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadBuffer));
		DXContext::Get().GetDevice()->CreateCommittedResource(&hpDefault, D3D12_HEAP_FLAG_NONE, &rd, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&vertexBuffer));

		// Copy void* -> CPU resource
		void* uploadBufferAddress;
		D3D12_RANGE uploadRange;

		uploadRange.Begin = 0;
		uploadRange.End = 1023;

		uploadBuffer->Map(0, &uploadRange, &uploadBufferAddress);
		memcpy(uploadBufferAddress, vertices, sizeof(vertices));
		uploadBuffer->Unmap(0, &uploadRange);

		// Copy CPU -> GPU 
		auto* cmdList = DXContext::Get().InitCommandList();
		cmdList->CopyBufferRegion(vertexBuffer, 0, uploadBuffer, 0, 1024);
		DXContext::Get().ExecuteCommandList();

		D3D12_GRAPHICS_PIPELINE_STATE_DESC gfxPsod{};
		gfxPsod.InputLayout.NumElements = _countof(vertexLayout) ;
		gfxPsod.InputLayout.pInputElementDescs = vertexLayout;
		gfxPsod.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;


		D3D12_VERTEX_BUFFER_VIEW vbv{};
		vbv.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
		vbv.SizeInBytes = sizeof(Vertex) * _countof(vertices);
		vbv.StrideInBytes = sizeof(Vertex); 


		// DXContext::Get().GetDevice()->CreatePipelineState();

		DXWindow::Get().SetFullscreen(true);
		while (!DXWindow::Get().ShouldClose()) {
			
			// Process pending window messages
			DXWindow::Get().Update();

			// Check and resize
			if (DXWindow::Get().ShouldResize()) {
				DXContext::Get().Flush(DXWindow::Get().GetFrameCount());
				DXWindow::Get().Resize();
			}

			// Begin drawing
			cmdList = DXContext::Get().InitCommandList();

			// Draw to window
			DXWindow::Get().BeginFrame(cmdList);
			
			// --- Input Assembler ---
			cmdList->IASetVertexBuffers(0, 1, &vbv);
			cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			// Draw 
			cmdList->DrawInstanced(_countof(vertices), 1, 0, 0);

			DXWindow::Get().EndFrame(cmdList);

			//Finish drawing and present
			DXContext::Get().ExecuteCommandList();
			DXWindow::Get().Preset(); 
		}

		//Need to flush the frame here, because once we exit the game loop(while loop above), Gpu/ Command queue has reference to the swap chain becuase it is still doing something, but if we release swapchain then it causes problems.
		DXContext::Get().Flush(DXWindow::GetFrameCount());

		//
		vertexBuffer.Release();
		uploadBuffer.Release(); 


		DXWindow::Get().Shutdown();
		DXContext::Get().Shutdown();
	}
	DXDebugLayer::Get().Shutdown();

}
#include <iostream>
#include "Support/WinInclude.h"
#include "Support/ComPointer.h"
#include "Support/Window.h"
#include "Support/Shader.h"
#include "Debug/DXDebugLayer.h"

#include "D3D/DXContext.h"

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

		// ==== Upload & Vertex Buffer ====
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

		// ==== Shader ====
		Shader rootSignatureShader("RootSignature.cso");
		Shader vertexShader("VertexShader.cso");
		Shader pixelShader("PixelShader.cso");

		// === Create root signature ===
		ComPointer<ID3D12RootSignature> rootSignature;
		DXContext::Get().GetDevice()->CreateRootSignature(0, rootSignatureShader.GetBuffer(), rootSignatureShader.GetSize(), IID_PPV_ARGS(&rootSignature));


		// ==== Pipeline State ====
		D3D12_GRAPHICS_PIPELINE_STATE_DESC gfxPsod{};
		gfxPsod.pRootSignature = rootSignature;
		gfxPsod.InputLayout.NumElements = _countof(vertexLayout) ;
		gfxPsod.InputLayout.pInputElementDescs = vertexLayout;
		gfxPsod.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
		gfxPsod.VS.BytecodeLength = vertexShader.GetSize();
		gfxPsod.VS.pShaderBytecode = vertexShader.GetBuffer();
		gfxPsod.PS.BytecodeLength = pixelShader.GetSize();
		gfxPsod.PS.pShaderBytecode = pixelShader.GetBuffer();
		gfxPsod.DS.BytecodeLength = 0;
		gfxPsod.DS.pShaderBytecode = nullptr;
		gfxPsod.HS.BytecodeLength = 0;
		gfxPsod.HS.pShaderBytecode = nullptr;
		gfxPsod.GS.BytecodeLength = 0;
		gfxPsod.GS.pShaderBytecode = nullptr;
		gfxPsod.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		gfxPsod.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		gfxPsod.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
		gfxPsod.RasterizerState.FrontCounterClockwise = FALSE;
		gfxPsod.RasterizerState.DepthBias = 0;
		gfxPsod.RasterizerState.DepthBiasClamp = .0f;
		gfxPsod.RasterizerState.SlopeScaledDepthBias = .0f;
		gfxPsod.RasterizerState.DepthClipEnable = FALSE;
		gfxPsod.RasterizerState.MultisampleEnable = FALSE;
		gfxPsod.RasterizerState.AntialiasedLineEnable = FALSE;
		gfxPsod.RasterizerState.ForcedSampleCount = 0;
		gfxPsod.StreamOutput.NumEntries = 0;
		gfxPsod.StreamOutput.NumStrides = 0;
		gfxPsod.StreamOutput.pBufferStrides = nullptr;
		gfxPsod.StreamOutput.pSODeclaration = nullptr;
		gfxPsod.StreamOutput.RasterizedStream = 0;

		gfxPsod.NumRenderTargets = 1;
		gfxPsod.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		gfxPsod.DSVFormat = DXGI_FORMAT_UNKNOWN;
		gfxPsod.BlendState.AlphaToCoverageEnable = FALSE;
		gfxPsod.BlendState.IndependentBlendEnable = FALSE;

		gfxPsod.BlendState.RenderTarget[0].BlendEnable = TRUE;
		gfxPsod.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
		gfxPsod.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
		gfxPsod.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
		gfxPsod.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		gfxPsod.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ZERO;
		gfxPsod.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
		gfxPsod.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		gfxPsod.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
		gfxPsod.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		gfxPsod.DepthStencilState.DepthEnable = FALSE;
		gfxPsod.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		gfxPsod.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		gfxPsod.DepthStencilState.StencilEnable = FALSE;
		gfxPsod.DepthStencilState.StencilReadMask = 0;
		gfxPsod.DepthStencilState.StencilWriteMask = 0;
		gfxPsod.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		gfxPsod.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		gfxPsod.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		gfxPsod.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
		gfxPsod.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		gfxPsod.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		gfxPsod.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		gfxPsod.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
		
		gfxPsod.SampleMask = 0xFFFFFFFF;
		gfxPsod.SampleDesc.Count = 1;
		gfxPsod.SampleDesc.Quality = 0;

		gfxPsod.NodeMask = 0;
		gfxPsod.CachedPSO.CachedBlobSizeInBytes = 0;
		gfxPsod.CachedPSO.pCachedBlob = nullptr;
		gfxPsod.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;


		ComPointer<ID3D12PipelineState> pso;
		DXContext::Get().GetDevice()->CreateGraphicsPipelineState(&gfxPsod, IID_PPV_ARGS(&pso));

		// ==== Vertex Buffer View====
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
			
			// === PSO === 
			cmdList->SetPipelineState(pso);
			cmdList->SetGraphicsRootSignature(rootSignature);

			// --- Input Assembler ---
			cmdList->IASetVertexBuffers(0, 1, &vbv);
			cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			// === Rasterizer State ===
			D3D12_VIEWPORT vp;
			vp.TopLeftX = vp.TopLeftY = 0;
			vp.Width = DXWindow::Get().GetWidth();
			vp.Height = DXWindow::Get().GetHeight();
			vp.MinDepth = 1.f;
			vp.MaxDepth = 0.f;
			cmdList->RSSetViewports(1, &vp);
			
			RECT scRect;
			scRect.left = scRect.top = 0;
			scRect.right = DXWindow::Get().GetWidth();
			scRect.bottom = DXWindow::Get().GetHeight();
			cmdList->RSSetScissorRects(1, &scRect);

			// === Root ===
			static float color[] = { 0.0f, 0.0f, 0.0f };
			cmdList->SetGraphicsRoot32BitConstants(0, 3, color, 0);

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
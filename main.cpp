#include <iostream>
#include"WinInclude.h"
#include "ComPointer.h"
#include "Window.h"

#include "DXDebugLayer.h"

#include "DXContext.h"

void main() {
	DXDebugLayer::Get().Init();

	if (DXContext::Get().Init() && DXWindow::Get().Init()) {
		
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
			auto* cmdList = DXContext::Get().InitCommandList();

			// Setup
			// Draw 
			DXWindow::Get().BeginFrame(cmdList);

			DXWindow::Get().EndFrame(cmdList);

			//Finish drawing and present
			DXContext::Get().ExecuteCommandList();
			DXWindow::Get().Preset(); 
		}

		//Need to flush the frame here, because once we exit the game loop(while loop above), Gpu/ Command queue has reference to the swap chain becuase it is still doing something, but if we release swapchain then it causes problems.
		DXContext::Get().Flush(DXWindow::GetFrameCount());

		DXWindow::Get().Shutdown();
		DXContext::Get().Shutdown();
	}
	DXDebugLayer::Get().Shutdown();

}
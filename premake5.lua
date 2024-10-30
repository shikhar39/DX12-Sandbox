workspace "DX12-projects"
    configurations {"Debug", "Release"}

    project "DX12-Sandbox"
        
        kind "ConsoleApp"
        language "C++"
        cppdialect "C++20"

        targetdir "bin/%{cfg.buildcfg}"

        files { "./src/**.h", "./src/**.cpp" }

        links { "d3d12", "dxgi", "d3dcompiler", "dxguid" }
        filter "configurations:Debug"
            defines { "DEBUG" }
            symbols "On"

        filter "configurations:Release"
            defines { "NDEBUG" }
            optimize "On"
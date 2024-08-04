cmake_minimum_required(VERSION 3.28)
project(LoFiGfx)

set(Source_List
        Third/volk/volk.c
        Source/VmaLoader.cpp
        Source/LoFiGfx.cpp
        Source/GfxContext.cpp
        Source/Message.cpp
        Source/PhysicalDevice.cpp
        Source/Helper.cpp
        Source/GfxComponents/Swapchain.cpp
        Source/GfxComponents/Texture.cpp
        Source/GfxComponents/Program.cpp
        Source/GfxComponents/Buffer.cpp
        Source/GfxComponents/Defines.h
        Source/GfxComponents/Kernel.cpp
        Source/GfxComponents/Kernel.h
        Source/FrameGraph.cpp
        Source/FrameGraph.h
        Source/GfxComponents/Buffer3F.cpp
        Source/GfxComponents/Buffer3F.h
        Source/PfxContext.cpp
        Source/PfxContext.h
        Source/RenderNode.cpp
)

find_package(Vulkan REQUIRED)

add_library(LoFiGfx SHARED ${Source_List})
target_compile_definitions(LoFiGfx PRIVATE LOFI_GFX_DLL_EXPORT)
target_include_directories(LoFiGfx PUBLIC Include)
target_include_directories(LoFiGfx PRIVATE Third)
target_link_directories(LoFiGfx PRIVATE ThirdBin)

target_link_libraries(LoFiGfx PRIVATE Vulkan::Vulkan EnTT::EnTT mimalloc-static-secure mimalloc-redirect
        spirv-cross-core glslang glslang-default-resource-limits SPIRV SPIRV-Tools-opt SPIRV-Tools GenericCodeGen MachineIndependent
        msdfgen-core msdfgen-ext freetype)

target_compile_options(LoFiGfx PRIVATE /source-charset:utf-8)

file(COPY ThirdBin/mimalloc-redirect.dll DESTINATION ${CMAKE_SOURCE_DIR}/Binary/${CMAKE_BUILD_TYPE})





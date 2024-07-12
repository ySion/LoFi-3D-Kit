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
        Source/GfxComponents/Window.cpp
        Source/GfxComponents/Swapchain.cpp
        Source/GfxComponents/Texture.cpp
        Source/GfxComponents/Program.cpp
        Source/GfxComponents/Buffer.cpp
        Source/GfxComponents/Defines.h
        Source/GfxComponents/ResourcePushConstant.cpp
        Source/GfxComponents/ResourcePushConstant.h
        Source/GfxComponents/Kernel.cpp
        Source/GfxComponents/Kernel.h
        Source/GfxComponents/KernelInstance.cpp
        Source/GfxComponents/KernelInstance.h
        Source/FrameGraph.cpp
        Source/FrameGraph.h
        Source/GfxComponents/FrameResource.cpp
        Source/GfxComponents/FrameResource.h
        Source/PfxContext.cpp
        Source/PfxContext.h
        Source/PtxComponents/RenderNode.cpp
)

find_package(Vulkan REQUIRED)

add_library(LoFiGfx SHARED ${Source_List})
target_compile_definitions(LoFiGfx PRIVATE LOFI_GFX_DLL_EXPORT)
target_include_directories(LoFiGfx PUBLIC Include)
target_include_directories(LoFiGfx PRIVATE Third)
target_link_directories(LoFiGfx PRIVATE ThirdBin)

target_link_libraries(LoFiGfx PRIVATE Vulkan::Vulkan SDL3::SDL3 EnTT::EnTT mimalloc-secure
        spirv-cross-core
        glslang glslang-default-resource-limits SPIRV SPIRV-Tools-opt SPIRV-Tools GenericCodeGen MachineIndependent
        msdf-atlas-gen msdfgen-core  msdfgen-ext freetype Clipper2)

target_compile_options(LoFiGfx PRIVATE /source-charset:utf-8)

file(COPY ThirdBin/mimalloc-redirect.dll DESTINATION ${CMAKE_SOURCE_DIR}/Binary/${CMAKE_BUILD_TYPE})
file(COPY ThirdBin/mimalloc-secure.dll DESTINATION ${CMAKE_SOURCE_DIR}/Binary/${CMAKE_BUILD_TYPE})





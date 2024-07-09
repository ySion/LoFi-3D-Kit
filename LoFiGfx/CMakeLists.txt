cmake_minimum_required(VERSION 3.28)
project(LoFiGfx)

set(Source_List
        Third/volk/volk.c
        Source/VmaLoader.cpp
        Source/LoFiGfx.cpp
        Source/Context.cpp
        Source/Message.cpp
        Source/PhysicalDevice.cpp
        Source/Helper.cpp
        Source/Components/Window.cpp
        Source/Components/Swapchain.cpp
        Source/Components/Texture.cpp
        Source/Components/Program.cpp
        Source/Components/Buffer.cpp
        Source/Components/Defines.h
        Source/Components/ResourcePushConstant.cpp
        Source/Components/ResourcePushConstant.h
        Source/Components/Kernel.cpp
        Source/Components/Kernel.h
        Source/Components/KernelInstance.cpp
        Source/Components/KernelInstance.h
        Source/FrameGraph.cpp
        Source/FrameGraph.h
        Source/Components/FrameResource.cpp
        Source/Components/FrameResource.h
)

find_package(Vulkan REQUIRED)

add_library(LoFiGfx SHARED ${Source_List})
target_compile_definitions(LoFiGfx PRIVATE LOFI_GFX_DLL_EXPORT)
target_include_directories(LoFiGfx PUBLIC Include)
target_include_directories(LoFiGfx PRIVATE Third)
target_link_directories(LoFiGfx PRIVATE ThirdBin)
target_link_libraries(LoFiGfx PRIVATE Vulkan::Vulkan SDL3::SDL3 EnTT::EnTT
        spirv-cross-core
        glslang glslang-default-resource-limits SPIRV SPIRV-Tools-opt SPIRV-Tools GenericCodeGen MachineIndependent)

target_compile_options(LoFiGfx PRIVATE /source-charset:utf-8)



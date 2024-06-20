cmake_minimum_required(VERSION 3.28)
project(LoFiGfx)

set(Source_List
        Third/volk/volk.c
        Source/VmaLoader.cpp
        Source/LoFiGfx.cpp
        Source/Context.cpp
        Source/Message.cpp
        Source/PhysicalDevice.cpp
)

find_package(Vulkan REQUIRED)

add_library(LoFiGfx SHARED ${Source_List})
target_compile_definitions(LoFiGfx PRIVATE LOFI_GFX_DLL_EXPORT)
target_include_directories(LoFiGfx PUBLIC Include)
target_link_libraries(LoFiGfx PRIVATE Vulkan::Vulkan SDL3::SDL3)

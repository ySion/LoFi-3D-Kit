//
// Created by starr on 2024/6/20.
//

#include "Helper.h"

namespace LoFi::Internal {

      static VmaAllocator loadedAllocator = VK_NULL_HANDLE;
      static entt::registry* loadedEcsWorld = nullptr;
      static VkPhysicalDevice loadedPhysicalDevice = VK_NULL_HANDLE;

      VmaAllocator volkGetLoadedVmaAllocator() {
            return loadedAllocator;
      }

      void volkLoadVmaAllocator(VmaAllocator allocator) {
            loadedAllocator = allocator;
      }

      entt::registry* volkGetLoadedEcsWorld() {
            return loadedEcsWorld;
      }

      void volkLoadEcsWorld(entt::registry* world) {
            loadedEcsWorld = world;
      }

      VkPhysicalDevice volkGetLoadedPhysicalDevice() {
            return loadedPhysicalDevice;
      }

      void volkLoadPhysicalDevice(VkPhysicalDevice device) {
            loadedPhysicalDevice = device;
      }

      const char* GetVkResultString(VkResult res) {
            switch (res) {
                  case VK_SUCCESS: return "VK_SUCCESS";
                  case VK_NOT_READY: return "VK_NOT_READY";
                  case VK_TIMEOUT: return "VK_TIMEOUT";
                  case VK_EVENT_SET: return "VK_EVENT_SET";
                  case VK_EVENT_RESET: return "VK_EVENT_RESET";
                  case VK_INCOMPLETE: return "VK_INCOMPLETE";
                  case VK_ERROR_OUT_OF_HOST_MEMORY: return "VK_ERROR_OUT_OF_HOST_MEMORY";
                  case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
                  case VK_ERROR_INITIALIZATION_FAILED: return "VK_ERROR_INITIALIZATION_FAILED";
                  case VK_ERROR_DEVICE_LOST: return "VK_ERROR_DEVICE_LOST";
                  case VK_ERROR_MEMORY_MAP_FAILED: return "VK_ERROR_MEMORY_MAP_FAILED";
                  case VK_ERROR_LAYER_NOT_PRESENT: return "VK_ERROR_LAYER_NOT_PRESENT";
                  case VK_ERROR_EXTENSION_NOT_PRESENT: return "VK_ERROR_EXTENSION_NOT_PRESENT";
                  case VK_ERROR_FEATURE_NOT_PRESENT: return "VK_ERROR_FEATURE_NOT_PRESENT";
                  case VK_ERROR_INCOMPATIBLE_DRIVER: return "VK_ERROR_INCOMPATIBLE_DRIVER";
                  case VK_ERROR_TOO_MANY_OBJECTS: return "VK_ERROR_TOO_MANY_OBJECTS";
                  case VK_ERROR_FORMAT_NOT_SUPPORTED: return "VK_ERROR_FORMAT_NOT_SUPPORTED";
                  case VK_ERROR_FRAGMENTED_POOL: return "VK_ERROR_FRAGMENTED_POOL";
                  case VK_ERROR_UNKNOWN: return "VK_ERROR_UNKNOWN";
                  case VK_ERROR_OUT_OF_POOL_MEMORY: return "VK_ERROR_OUT_OF_POOL_MEMORY";
                  case VK_ERROR_INVALID_EXTERNAL_HANDLE: return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
                  case VK_ERROR_FRAGMENTATION: return "VK_ERROR_FRAGMENTATION";
                  case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS: return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
                  case VK_PIPELINE_COMPILE_REQUIRED: return "VK_PIPELINE_COMPILE_REQUIRED";
                  case VK_ERROR_SURFACE_LOST_KHR: return "VK_ERROR_SURFACE_LOST_KHR";
                  case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
                  case VK_SUBOPTIMAL_KHR: return "VK_SUBOPTIMAL_KHR";
                  case VK_ERROR_OUT_OF_DATE_KHR: return "VK_ERROR_OUT_OF_DATE_KHR";
                  case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
                  case VK_ERROR_VALIDATION_FAILED_EXT: return "VK_ERROR_VALIDATION_FAILED_EXT";
                  case VK_ERROR_INVALID_SHADER_NV: return "VK_ERROR_INVALID_SHADER_NV";
                  case VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR: return "VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR";
                  case VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR: return
                              "VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR";
                  case VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR: return
                              "VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR";
                  case VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR: return
                              "VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR";
                  case VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR: return
                              "VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR";
                  case VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR: return "VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR";
                  case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT: return
                              "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
                  case VK_ERROR_NOT_PERMITTED_KHR: return "VK_ERROR_NOT_PERMITTED_KHR";
                  case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT: return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
                  case VK_THREAD_IDLE_KHR: return "VK_THREAD_IDLE_KHR";
                  case VK_THREAD_DONE_KHR: return "VK_THREAD_DONE_KHR";
                  case VK_OPERATION_DEFERRED_KHR: return "VK_OPERATION_DEFERRED_KHR";
                  case VK_OPERATION_NOT_DEFERRED_KHR: return "VK_OPERATION_NOT_DEFERRED_KHR";
                  case VK_ERROR_COMPRESSION_EXHAUSTED_EXT: return "VK_ERROR_COMPRESSION_EXHAUSTED_EXT";
                  case VK_ERROR_INCOMPATIBLE_SHADER_BINARY_EXT: return "VK_ERROR_COMPRESSION_EXHAUSTED_EXT";
                  defualt :
                        return "UNKNOWN";
            }
            return "UNKNOWN";
      }

      const char* GetVkFormatString(VkFormat format) {
            switch (format) {
                  default: return "VK_FORMAT_UNDEFINED";
                  case VK_FORMAT_R4G4_UNORM_PACK8: return "VK_FORMAT_R4G4_UNORM_PACK8";
                  case VK_FORMAT_R4G4B4A4_UNORM_PACK16: return "VK_FORMAT_R4G4B4A4_UNORM_PACK16";
                  case VK_FORMAT_B4G4R4A4_UNORM_PACK16: return "VK_FORMAT_B4G4R4A4_UNORM_PACK16";
                  case VK_FORMAT_R5G6B5_UNORM_PACK16: return "VK_FORMAT_R5G6B5_UNORM_PACK16";
                  case VK_FORMAT_B5G6R5_UNORM_PACK16: return "VK_FORMAT_B5G6R5_UNORM_PACK16";
                  case VK_FORMAT_R5G5B5A1_UNORM_PACK16: return "VK_FORMAT_R5G5B5A1_UNORM_PACK16";
                  case VK_FORMAT_B5G5R5A1_UNORM_PACK16: return "VK_FORMAT_B5G5R5A1_UNORM_PACK16";
                  case VK_FORMAT_A1R5G5B5_UNORM_PACK16: return "VK_FORMAT_A1R5G5B5_UNORM_PACK16";
                  case VK_FORMAT_R8_UNORM: return "VK_FORMAT_R8_UNORM";
                  case VK_FORMAT_R8_SNORM: return "VK_FORMAT_R8_SNORM";
                  case VK_FORMAT_R8_USCALED: return "VK_FORMAT_R8_USCALED";
                  case VK_FORMAT_R8_SSCALED: return "VK_FORMAT_R8_SSCALED";
                  case VK_FORMAT_R8_UINT: return "VK_FORMAT_R8_UINT";
                  case VK_FORMAT_R8_SINT: return "VK_FORMAT_R8_SINT";
                  case VK_FORMAT_R8_SRGB: return "VK_FORMAT_R8_SRGB";
                  case VK_FORMAT_R8G8_UNORM: return "VK_FORMAT_R8G8_UNORM";
                  case VK_FORMAT_R8G8_SNORM: return "VK_FORMAT_R8G8_SNORM";
                  case VK_FORMAT_R8G8_USCALED: return "VK_FORMAT_R8G8_USCALED";
                  case VK_FORMAT_R8G8_SSCALED: return "VK_FORMAT_R8G8_SSCALED";
                  case VK_FORMAT_R8G8_UINT: return "VK_FORMAT_R8G8_UINT";
                  case VK_FORMAT_R8G8_SINT: return "VK_FORMAT_R8G8_SINT";
                  case VK_FORMAT_R8G8_SRGB: return "VK_FORMAT_R8G8_SRGB";
                  case VK_FORMAT_R8G8B8_UNORM: return "VK_FORMAT_R8G8B8_UNORM";
                  case VK_FORMAT_R8G8B8_SNORM: return "VK_FORMAT_R8G8B8_SNORM";
                  case VK_FORMAT_R8G8B8_USCALED: return "VK_FORMAT_R8G8B8_USCALED";
                  case VK_FORMAT_R8G8B8_SSCALED: return "VK_FORMAT_R8G8B8_SSCALED";
                  case VK_FORMAT_R8G8B8_UINT: return "VK_FORMAT_R8G8B8_UINT";
                  case VK_FORMAT_R8G8B8_SINT: return "VK_FORMAT_R8G8B8_SINT";
                  case VK_FORMAT_R8G8B8_SRGB: return "VK_FORMAT_R8G8B8_SRGB";
                  case VK_FORMAT_B8G8R8_UNORM: return "VK_FORMAT_B8G8R8_UNORM";
                  case VK_FORMAT_B8G8R8_SNORM: return "VK_FORMAT_B8G8R8_SNORM";
                  case VK_FORMAT_B8G8R8_USCALED: return "VK_FORMAT_B8G8R8_USCALED";
                  case VK_FORMAT_B8G8R8_SSCALED: return "VK_FORMAT_B8G8R8_SSCALED";
                  case VK_FORMAT_B8G8R8_UINT: return "VK_FORMAT_B8G8R8_UINT";
                  case VK_FORMAT_B8G8R8_SINT: return "VK_FORMAT_B8G8R8_SINT";
                  case VK_FORMAT_B8G8R8_SRGB: return "VK_FORMAT_B8G8R8_SRGB";
                  case VK_FORMAT_R8G8B8A8_UNORM: return "VK_FORMAT_R8G8B8A8_UNORM";
                  case VK_FORMAT_R8G8B8A8_SNORM: return "VK_FORMAT_R8G8B8A8_SNORM";
                  case VK_FORMAT_R8G8B8A8_USCALED: return "VK_FORMAT_R8G8B8A8_USCALED";
                  case VK_FORMAT_R8G8B8A8_SSCALED: return "VK_FORMAT_R8G8B8A8_SSCALED";
                  case VK_FORMAT_R8G8B8A8_UINT: return "VK_FORMAT_R8G8B8A8_UINT";
                  case VK_FORMAT_R8G8B8A8_SINT: return "VK_FORMAT_R8G8B8A8_SINT";
                  case VK_FORMAT_R8G8B8A8_SRGB: return "VK_FORMAT_R8G8B8A8_SRGB";
                  case VK_FORMAT_B8G8R8A8_UNORM: return "VK_FORMAT_B8G8R8A8_UNORM";
                  case VK_FORMAT_B8G8R8A8_SNORM: return "VK_FORMAT_B8G8R8A8_SNORM";
                  case VK_FORMAT_B8G8R8A8_USCALED: return "VK_FORMAT_B8G8R8A8_USCALED";
                  case VK_FORMAT_B8G8R8A8_SSCALED: return "VK_FORMAT_B8G8R8A8_SSCALED";
                  case VK_FORMAT_B8G8R8A8_UINT: return "VK_FORMAT_B8G8R8A8_UINT";
                  case VK_FORMAT_B8G8R8A8_SINT: return "VK_FORMAT_B8G8R8A8_SINT";
                  case VK_FORMAT_B8G8R8A8_SRGB: return "VK_FORMAT_B8G8R8A8_SRGB";
                  case VK_FORMAT_A8B8G8R8_UNORM_PACK32: return "VK_FORMAT_A8B8G8R8_UNORM_PACK32";
                  case VK_FORMAT_A8B8G8R8_SNORM_PACK32: return "VK_FORMAT_A8B8G8R8_SNORM_PACK32";
                  case VK_FORMAT_A8B8G8R8_USCALED_PACK32: return "VK_FORMAT_A8B8G8R8_USCALED_PACK32";
                  case VK_FORMAT_A8B8G8R8_SSCALED_PACK32: return "VK_FORMAT_A8B8G8R8_SSCALED_PACK32";
                  case VK_FORMAT_A8B8G8R8_UINT_PACK32: return "VK_FORMAT_A8B8G8R8_UINT_PACK32";
                  case VK_FORMAT_A8B8G8R8_SINT_PACK32: return "VK_FORMAT_A8B8G8R8_SINT_PACK32";
                  case VK_FORMAT_A8B8G8R8_SRGB_PACK32: return "VK_FORMAT_A8B8G8R8_SRGB_PACK32";
                  case VK_FORMAT_A2R10G10B10_UNORM_PACK32: return "VK_FORMAT_A2R10G10B10_UNORM_PACK32";
                  case VK_FORMAT_A2R10G10B10_SNORM_PACK32: return "VK_FORMAT_A2R10G10B10_SNORM_PACK32";
                  case VK_FORMAT_A2R10G10B10_USCALED_PACK32: return "VK_FORMAT_A2R10G10B10_USCALED_PACK32";
                  case VK_FORMAT_A2R10G10B10_SSCALED_PACK32: return "VK_FORMAT_A2R10G10B10_SSCALED_PACK32";
                  case VK_FORMAT_A2R10G10B10_UINT_PACK32: return "VK_FORMAT_A2R10G10B10_UINT_PACK32";
                  case VK_FORMAT_A2R10G10B10_SINT_PACK32: return "VK_FORMAT_A2R10G10B10_SINT_PACK32";
                  case VK_FORMAT_A2B10G10R10_UNORM_PACK32: return "VK_FORMAT_A2B10G10R10_UNORM_PACK32";
                  case VK_FORMAT_A2B10G10R10_SNORM_PACK32: return "VK_FORMAT_A2B10G10R10_SNORM_PACK32";
                  case VK_FORMAT_A2B10G10R10_USCALED_PACK32: return "VK_FORMAT_A2B10G10R10_USCALED_PACK32";
                  case VK_FORMAT_A2B10G10R10_SSCALED_PACK32: return "VK_FORMAT_A2B10G10R10_SSCALED_PACK32";
                  case VK_FORMAT_A2B10G10R10_UINT_PACK32: return "VK_FORMAT_A2B10G10R10_UINT_PACK32";
                  case VK_FORMAT_A2B10G10R10_SINT_PACK32: return "VK_FORMAT_A2B10G10R10_SINT_PACK32";
                  case VK_FORMAT_R16_UNORM: return "VK_FORMAT_R16_UNORM";
                  case VK_FORMAT_R16_SNORM: return "VK_FORMAT_R16_SNORM";
                  case VK_FORMAT_R16_USCALED: return "VK_FORMAT_R16_USCALED";
                  case VK_FORMAT_R16_SSCALED: return "VK_FORMAT_R16_SSCALED";
                  case VK_FORMAT_R16_UINT: return "VK_FORMAT_R16_UINT";
                  case VK_FORMAT_R16_SINT: return "VK_FORMAT_R16_SINT";
                  case VK_FORMAT_R16_SFLOAT: return "VK_FORMAT_R16_SFLOAT";
                  case VK_FORMAT_R16G16_UNORM: return "VK_FORMAT_R16G16_UNORM";
                  case VK_FORMAT_R16G16_SNORM: return "VK_FORMAT_R16G16_SNORM";
                  case VK_FORMAT_R16G16_USCALED: return "VK_FORMAT_R16G16_USCALED";
                  case VK_FORMAT_R16G16_SSCALED: return "VK_FORMAT_R16G16_SSCALED";
                  case VK_FORMAT_R16G16_UINT: return "VK_FORMAT_R16G16_UINT";
                  case VK_FORMAT_R16G16_SINT: return "VK_FORMAT_R16G16_SINT";
                  case VK_FORMAT_R16G16_SFLOAT: return "VK_FORMAT_R16G16_SFLOAT";
                  case VK_FORMAT_R16G16B16_UNORM: return "VK_FORMAT_R16G16B16_UNORM";
                  case VK_FORMAT_R16G16B16_SNORM: return "VK_FORMAT_R16G16B16_SNORM";
                  case VK_FORMAT_R16G16B16_USCALED: return "VK_FORMAT_R16G16B16_USCALED";
                  case VK_FORMAT_R16G16B16_SSCALED: return "VK_FORMAT_R16G16B16_SSCALED";
                  case VK_FORMAT_R16G16B16_UINT: return "VK_FORMAT_R16G16B16_UINT";
                  case VK_FORMAT_R16G16B16_SINT: return "VK_FORMAT_R16G16B16_SINT";
                  case VK_FORMAT_R16G16B16_SFLOAT: return "VK_FORMAT_R16G16B16_SFLOAT";
                  case VK_FORMAT_R16G16B16A16_UNORM: return "VK_FORMAT_R16G16B16A16_UNORM";
                  case VK_FORMAT_R16G16B16A16_SNORM: return "VK_FORMAT_R16G16B16A16_SNORM";
                  case VK_FORMAT_R16G16B16A16_USCALED: return "VK_FORMAT_R16G16B16A16_USCALED";
                  case VK_FORMAT_R16G16B16A16_SSCALED: return "VK_FORMAT_R16G16B16A16_SSCALED";
                  case VK_FORMAT_R16G16B16A16_UINT: return "VK_FORMAT_R16G16B16A16_UINT";
                  case VK_FORMAT_R16G16B16A16_SINT: return "VK_FORMAT_R16G16B16A16_SINT";
                  case VK_FORMAT_R16G16B16A16_SFLOAT: return "VK_FORMAT_R16G16B16A16_SFLOAT";
                  case VK_FORMAT_R32_UINT: return "VK_FORMAT_R32_UINT";
                  case VK_FORMAT_R32_SINT: return "VK_FORMAT_R32_SINT";
                  case VK_FORMAT_R32_SFLOAT: return "VK_FORMAT_R32_SFLOAT";
                  case VK_FORMAT_R32G32_UINT: return "VK_FORMAT_R32G32_UINT";
                  case VK_FORMAT_R32G32_SINT: return "VK_FORMAT_R32G32_SINT";
                  case VK_FORMAT_R32G32_SFLOAT: return "VK_FORMAT_R32G32_SFLOAT";
                  case VK_FORMAT_R32G32B32_UINT: return "VK_FORMAT_R32G32B32_UINT";
                  case VK_FORMAT_R32G32B32_SINT: return "VK_FORMAT_R32G32B32_SINT";
                  case VK_FORMAT_R32G32B32_SFLOAT: return "VK_FORMAT_R32G32B32_SFLOAT";
                  case VK_FORMAT_R32G32B32A32_UINT: return "VK_FORMAT_R32G32B32A32_UINT";
                  case VK_FORMAT_R32G32B32A32_SINT: return "VK_FORMAT_R32G32B32A32_SINT";
                  case VK_FORMAT_R32G32B32A32_SFLOAT: return "VK_FORMAT_R32G32B32A32_SFLOAT";
                  case VK_FORMAT_R64_UINT: return "VK_FORMAT_R64_UINT";
                  case VK_FORMAT_R64_SINT: return "VK_FORMAT_R64_SINT";
                  case VK_FORMAT_R64_SFLOAT: return "VK_FORMAT_R64_SFLOAT";
                  case VK_FORMAT_R64G64_UINT: return "VK_FORMAT_R64G64_UINT";
                  case VK_FORMAT_R64G64_SINT: return "VK_FORMAT_R64G64_SINT";
                  case VK_FORMAT_R64G64_SFLOAT: return "VK_FORMAT_R64G64_SFLOAT";
                  case VK_FORMAT_R64G64B64_UINT: return "VK_FORMAT_R64G64B64_UINT";
                  case VK_FORMAT_R64G64B64_SINT: return "VK_FORMAT_R64G64B64_SINT";
                  case VK_FORMAT_R64G64B64_SFLOAT: return "VK_FORMAT_R64G64B64_SFLOAT";
                  case VK_FORMAT_R64G64B64A64_UINT: return "VK_FORMAT_R64G64B64A64_UINT";
                  case VK_FORMAT_R64G64B64A64_SINT: return "VK_FORMAT_R64G64B64A64_SINT";
                  case VK_FORMAT_R64G64B64A64_SFLOAT: return "VK_FORMAT_R64G64B64A64_SFLOAT";
                  case VK_FORMAT_B10G11R11_UFLOAT_PACK32: return "VK_FORMAT_B10G11R11_UFLOAT_PACK32";
                  case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32: return "VK_FORMAT_E5B9G9R9_UFLOAT_PACK32";
                  case VK_FORMAT_D16_UNORM: return "VK_FORMAT_D16_UNORM";
                  case VK_FORMAT_X8_D24_UNORM_PACK32: return "VK_FORMAT_X8_D24_UNORM_PACK32";
                  case VK_FORMAT_D32_SFLOAT: return "VK_FORMAT_D32_SFLOAT";
                  case VK_FORMAT_S8_UINT: return "VK_FORMAT_S8_UINT";
                  case VK_FORMAT_D16_UNORM_S8_UINT: return "VK_FORMAT_D16_UNORM_S8_UINT";
                  case VK_FORMAT_D24_UNORM_S8_UINT: return "VK_FORMAT_D24_UNORM_S8_UINT";
                  case VK_FORMAT_D32_SFLOAT_S8_UINT: return "VK_FORMAT_D32_SFLOAT_S8_UINT";
                  case VK_FORMAT_BC1_RGB_UNORM_BLOCK: return "VK_FORMAT_BC1_RGB_UNORM_BLOCK";
                  case VK_FORMAT_BC1_RGB_SRGB_BLOCK: return "VK_FORMAT_BC1_RGB_SRGB_BLOCK";
                  case VK_FORMAT_BC1_RGBA_UNORM_BLOCK: return "VK_FORMAT_BC1_RGBA_UNORM_BLOCK";
                  case VK_FORMAT_BC1_RGBA_SRGB_BLOCK: return "VK_FORMAT_BC1_RGBA_SRGB_BLOCK";
                  case VK_FORMAT_BC2_UNORM_BLOCK: return "VK_FORMAT_BC2_UNORM_BLOCK";
                  case VK_FORMAT_BC2_SRGB_BLOCK: return "VK_FORMAT_BC2_SRGB_BLOCK";
                  case VK_FORMAT_BC3_UNORM_BLOCK: return "VK_FORMAT_BC3_UNORM_BLOCK";
                  case VK_FORMAT_BC3_SRGB_BLOCK: return "VK_FORMAT_BC3_SRGB_BLOCK";
                  case VK_FORMAT_BC4_UNORM_BLOCK: return "VK_FORMAT_BC4_UNORM_BLOCK";
                  case VK_FORMAT_BC4_SNORM_BLOCK: return "VK_FORMAT_BC4_SNORM_BLOCK";
                  case VK_FORMAT_BC5_UNORM_BLOCK: return "VK_FORMAT_BC5_UNORM_BLOCK";
                  case VK_FORMAT_BC5_SNORM_BLOCK: return "VK_FORMAT_BC5_SNORM_BLOCK";
                  case VK_FORMAT_BC6H_UFLOAT_BLOCK: return "VK_FORMAT_BC6H_UFLOAT_BLOCK";
                  case VK_FORMAT_BC6H_SFLOAT_BLOCK: return "VK_FORMAT_BC6H_SFLOAT_BLOCK";
                  case VK_FORMAT_BC7_UNORM_BLOCK: return "VK_FORMAT_BC7_UNORM_BLOCK";
                  case VK_FORMAT_BC7_SRGB_BLOCK: return "VK_FORMAT_BC7_SRGB_BLOCK";
                  case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK: return "VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK";
                  case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK: return "VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK";
                  case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK: return "VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK";
                  case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK: return "VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK";
                  case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK: return "VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK";
                  case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK: return "VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK";
                  case VK_FORMAT_EAC_R11_UNORM_BLOCK: return "VK_FORMAT_EAC_R11_UNORM_BLOCK";
                  case VK_FORMAT_EAC_R11_SNORM_BLOCK: return "VK_FORMAT_EAC_R11_SNORM_BLOCK";
                  case VK_FORMAT_EAC_R11G11_UNORM_BLOCK: return "VK_FORMAT_EAC_R11G11_UNORM_BLOCK";
                  case VK_FORMAT_EAC_R11G11_SNORM_BLOCK: return "VK_FORMAT_EAC_R11G11_SNORM_BLOCK";
                  case VK_FORMAT_ASTC_4x4_UNORM_BLOCK: return "VK_FORMAT_ASTC_4x4_UNORM_BLOCK";
                  case VK_FORMAT_ASTC_4x4_SRGB_BLOCK: return "VK_FORMAT_ASTC_4x4_SRGB_BLOCK";
                  case VK_FORMAT_ASTC_5x4_UNORM_BLOCK: return "VK_FORMAT_ASTC_5x4_UNORM_BLOCK";
                  case VK_FORMAT_ASTC_5x4_SRGB_BLOCK: return "VK_FORMAT_ASTC_5x4_SRGB_BLOCK";
                  case VK_FORMAT_ASTC_5x5_UNORM_BLOCK: return "VK_FORMAT_ASTC_5x5_UNORM_BLOCK";
                  case VK_FORMAT_ASTC_5x5_SRGB_BLOCK: return "VK_FORMAT_ASTC_5x5_SRGB_BLOCK";
                  case VK_FORMAT_ASTC_6x5_UNORM_BLOCK: return "VK_FORMAT_ASTC_6x5_UNORM_BLOCK";
                  case VK_FORMAT_ASTC_6x5_SRGB_BLOCK: return "VK_FORMAT_ASTC_6x5_SRGB_BLOCK";
                  case VK_FORMAT_ASTC_6x6_UNORM_BLOCK: return "VK_FORMAT_ASTC_6x6_UNORM_BLOCK";
                  case VK_FORMAT_ASTC_6x6_SRGB_BLOCK: return "VK_FORMAT_ASTC_6x6_SRGB_BLOCK";
                  case VK_FORMAT_ASTC_8x5_UNORM_BLOCK: return "VK_FORMAT_ASTC_8x5_UNORM_BLOCK";
                  case VK_FORMAT_ASTC_8x5_SRGB_BLOCK: return "VK_FORMAT_ASTC_8x5_SRGB_BLOCK";
                  case VK_FORMAT_ASTC_8x6_UNORM_BLOCK: return "VK_FORMAT_ASTC_8x6_UNORM_BLOCK";
                  case VK_FORMAT_ASTC_8x6_SRGB_BLOCK: return "VK_FORMAT_ASTC_8x6_SRGB_BLOCK";
                  case VK_FORMAT_ASTC_8x8_UNORM_BLOCK: return "VK_FORMAT_ASTC_8x8_UNORM_BLOCK";
                  case VK_FORMAT_ASTC_8x8_SRGB_BLOCK: return "VK_FORMAT_ASTC_8x8_SRGB_BLOCK";
                  case VK_FORMAT_ASTC_10x5_UNORM_BLOCK: return "VK_FORMAT_ASTC_10x5_UNORM_BLOCK";
                  case VK_FORMAT_ASTC_10x5_SRGB_BLOCK: return "VK_FORMAT_ASTC_10x5_SRGB_BLOCK";
                  case VK_FORMAT_ASTC_10x6_UNORM_BLOCK: return "VK_FORMAT_ASTC_10x6_UNORM_BLOCK";
                  case VK_FORMAT_ASTC_10x6_SRGB_BLOCK: return "VK_FORMAT_ASTC_10x6_SRGB_BLOCK";
                  case VK_FORMAT_ASTC_10x8_UNORM_BLOCK: return "VK_FORMAT_ASTC_10x8_UNORM_BLOCK";
                  case VK_FORMAT_ASTC_10x8_SRGB_BLOCK: return "VK_FORMAT_ASTC_10x8_SRGB_BLOCK";
                  case VK_FORMAT_ASTC_10x10_UNORM_BLOCK: return "VK_FORMAT_ASTC_10x10_UNORM_BLOCK";
                  case VK_FORMAT_ASTC_10x10_SRGB_BLOCK: return "VK_FORMAT_ASTC_10x10_SRGB_BLOCK";
                  case VK_FORMAT_ASTC_12x10_UNORM_BLOCK: return "VK_FORMAT_ASTC_12x10_UNORM_BLOCK";
                  case VK_FORMAT_ASTC_12x10_SRGB_BLOCK: return "VK_FORMAT_ASTC_12x10_SRGB_BLOCK";
                  case VK_FORMAT_ASTC_12x12_UNORM_BLOCK: return "VK_FORMAT_ASTC_12x12_UNORM_BLOCK";
                  case VK_FORMAT_ASTC_12x12_SRGB_BLOCK: return "VK_FORMAT_ASTC_12x12_SRGB_BLOCK";
                  case VK_FORMAT_G8B8G8R8_422_UNORM: return "VK_FORMAT_G8B8G8R8_422_UNORM";
                  case VK_FORMAT_B8G8R8G8_422_UNORM: return "VK_FORMAT_B8G8R8G8_422_UNORM";
                  case VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM: return "VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM";
                  case VK_FORMAT_G8_B8R8_2PLANE_420_UNORM: return "VK_FORMAT_G8_B8R8_2PLANE_420_UNORM";
                  case VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM: return "VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM";
                  case VK_FORMAT_G8_B8R8_2PLANE_422_UNORM: return "VK_FORMAT_G8_B8R8_2PLANE_422_UNORM";
                  case VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM: return "VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM";
                  case VK_FORMAT_R10X6_UNORM_PACK16: return "VK_FORMAT_R10X6_UNORM_PACK16";
                  case VK_FORMAT_R10X6G10X6_UNORM_2PACK16: return "VK_FORMAT_R10X6G10X6_UNORM_2PACK16";
                  case VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16: return "VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16";
                  case VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16: return
                              "VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16";
                  case VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16: return
                              "VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16";
                  case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16: return
                              "VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16";
                  case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16: return
                              "VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16";
                  case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16: return
                              "VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16";
                  case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16: return
                              "VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16";
                  case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16: return
                              "VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16";
                  case VK_FORMAT_R12X4_UNORM_PACK16: return "VK_FORMAT_R12X4_UNORM_PACK16";
                  case VK_FORMAT_R12X4G12X4_UNORM_2PACK16: return "VK_FORMAT_R12X4G12X4_UNORM_2PACK16";
                  case VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16: return "VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16";
                  case VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16: return
                              "VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16";
                  case VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16: return
                              "VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16";
                  case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16: return
                              "VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16";
                  case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16: return
                              "VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16";
                  case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16: return
                              "VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16";
                  case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16: return
                              "VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16";
                  case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16: return
                              "VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16";
                  case VK_FORMAT_G16B16G16R16_422_UNORM: return "VK_FORMAT_G16B16G16R16_422_UNORM";
                  case VK_FORMAT_B16G16R16G16_422_UNORM: return "VK_FORMAT_B16G16R16G16_422_UNORM";
                  case VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM: return "VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM";
                  case VK_FORMAT_G16_B16R16_2PLANE_420_UNORM: return "VK_FORMAT_G16_B16R16_2PLANE_420_UNORM";
                  case VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM: return "VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM";
                  case VK_FORMAT_G16_B16R16_2PLANE_422_UNORM: return "VK_FORMAT_G16_B16R16_2PLANE_422_UNORM";
                  case VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM: return "VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM";
                  case VK_FORMAT_G8_B8R8_2PLANE_444_UNORM: return "VK_FORMAT_G8_B8R8_2PLANE_444_UNORM";
                  case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16: return
                              "VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16";
                  case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16: return
                              "VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16";
                  case VK_FORMAT_G16_B16R16_2PLANE_444_UNORM: return "VK_FORMAT_G16_B16R16_2PLANE_444_UNORM";
                  case VK_FORMAT_A4R4G4B4_UNORM_PACK16: return "VK_FORMAT_A4R4G4B4_UNORM_PACK16";
                  case VK_FORMAT_A4B4G4R4_UNORM_PACK16: return "VK_FORMAT_A4B4G4R4_UNORM_PACK16";
                  case VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK: return "VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK";
                  case VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK: return "VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK";
                  case VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK: return "VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK";
                  case VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK: return "VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK";
                  case VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK: return "VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK";
                  case VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK: return "VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK";
                  case VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK: return "VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK";
                  case VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK: return "VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK";
                  case VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK: return "VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK";
                  case VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK: return "VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK";
                  case VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK: return "VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK";
                  case VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK: return "VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK";
                  case VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK: return "VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK";
                  case VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK: return "VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK";
                  case VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG: return "VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG";
                  case VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG: return "VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG";
                  case VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG: return "VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG";
                  case VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG: return "VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG";
                  case VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG: return "VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG";
                  case VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG: return "VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG";
                  case VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG: return "VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG";
                  case VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG: return "VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG";
                  case VK_FORMAT_R16G16_S10_5_NV: return "VK_FORMAT_R16G16_S10_5_NV";
                  case VK_FORMAT_A1B5G5R5_UNORM_PACK16_KHR: return "VK_FORMAT_A1B5G5R5_UNORM_PACK16_KHR";
                  case VK_FORMAT_A8_UNORM_KHR: return "VK_FORMAT_A8_UNORM_KHR";
            }
      }

      const char* GetVkFormatStringSimpled(VkFormat format) {
            switch (format) {
                  default: return "VK_FORMAT_UNDEFINED";
                  case VK_FORMAT_R4G4_UNORM_PACK8: return "r4g4_unorm_pack8";
                  case VK_FORMAT_R4G4B4A4_UNORM_PACK16: return "r4g4b4a4_unorm_pack16";
                  case VK_FORMAT_B4G4R4A4_UNORM_PACK16: return "b4g4r4a4_unorm_pack16";
                  case VK_FORMAT_R5G6B5_UNORM_PACK16: return "r5g6b5_unorm_pack16";
                  case VK_FORMAT_B5G6R5_UNORM_PACK16: return "b5g6r5_unorm_pack16";
                  case VK_FORMAT_R5G5B5A1_UNORM_PACK16: return "r5g5b5a1_unorm_pack16";
                  case VK_FORMAT_B5G5R5A1_UNORM_PACK16: return "b5g5r5a1_unorm_pack16";
                  case VK_FORMAT_A1R5G5B5_UNORM_PACK16: return "a1r5g5b5_unorm_pack16";
                  case VK_FORMAT_R8_UNORM: return "r8_unorm";
                  case VK_FORMAT_R8_SNORM: return "r8_snorm";
                  case VK_FORMAT_R8_USCALED: return "r8_uscaled";
                  case VK_FORMAT_R8_SSCALED: return "r8_sscaled";
                  case VK_FORMAT_R8_UINT: return "r8_uint";
                  case VK_FORMAT_R8_SINT: return "r8_sint";
                  case VK_FORMAT_R8_SRGB: return "r8_srgb";
                  case VK_FORMAT_R8G8_UNORM: return "r8g8_unorm";
                  case VK_FORMAT_R8G8_SNORM: return "r8g8_snorm";
                  case VK_FORMAT_R8G8_USCALED: return "r8g8_uscaled";
                  case VK_FORMAT_R8G8_SSCALED: return "r8g8_sscaled";
                  case VK_FORMAT_R8G8_UINT: return "r8g8_uint";
                  case VK_FORMAT_R8G8_SINT: return "r8g8_sint";
                  case VK_FORMAT_R8G8_SRGB: return "r8g8_srgb";
                  case VK_FORMAT_R8G8B8_UNORM: return "r8g8b8_unorm";
                  case VK_FORMAT_R8G8B8_SNORM: return "r8g8b8_snorm";
                  case VK_FORMAT_R8G8B8_USCALED: return "r8g8b8_uscaled";
                  case VK_FORMAT_R8G8B8_SSCALED: return "r8g8b8_sscaled";
                  case VK_FORMAT_R8G8B8_UINT: return "r8g8b8_uint";
                  case VK_FORMAT_R8G8B8_SINT: return "r8g8b8_sint";
                  case VK_FORMAT_R8G8B8_SRGB: return "r8g8b8_srgb";
                  case VK_FORMAT_B8G8R8_UNORM: return "b8g8r8_unorm";
                  case VK_FORMAT_B8G8R8_SNORM: return "b8g8r8_snorm";
                  case VK_FORMAT_B8G8R8_USCALED: return "b8g8r8_uscaled";
                  case VK_FORMAT_B8G8R8_SSCALED: return "b8g8r8_sscaled";
                  case VK_FORMAT_B8G8R8_UINT: return "b8g8r8_uint";
                  case VK_FORMAT_B8G8R8_SINT: return "b8g8r8_sint";
                  case VK_FORMAT_B8G8R8_SRGB: return "b8g8r8_srgb";
                  case VK_FORMAT_R8G8B8A8_UNORM: return "r8g8b8a8_unorm";
                  case VK_FORMAT_R8G8B8A8_SNORM: return "r8g8b8a8_snorm";
                  case VK_FORMAT_R8G8B8A8_USCALED: return "r8g8b8a8_uscaled";
                  case VK_FORMAT_R8G8B8A8_SSCALED: return "r8g8b8a8_sscaled";
                  case VK_FORMAT_R8G8B8A8_UINT: return "r8g8b8a8_uint";
                  case VK_FORMAT_R8G8B8A8_SINT: return "r8g8b8a8_sint";
                  case VK_FORMAT_R8G8B8A8_SRGB: return "r8g8b8a8_srgb";
                  case VK_FORMAT_B8G8R8A8_UNORM: return "b8g8r8a8_unorm";
                  case VK_FORMAT_B8G8R8A8_SNORM: return "b8g8r8a8_snorm";
                  case VK_FORMAT_B8G8R8A8_USCALED: return "b8g8r8a8_uscaled";
                  case VK_FORMAT_B8G8R8A8_SSCALED: return "b8g8r8a8_sscaled";
                  case VK_FORMAT_B8G8R8A8_UINT: return "b8g8r8a8_uint";
                  case VK_FORMAT_B8G8R8A8_SINT: return "b8g8r8a8_sint";
                  case VK_FORMAT_B8G8R8A8_SRGB: return "b8g8r8a8_srgb";
                  case VK_FORMAT_A8B8G8R8_UNORM_PACK32: return "a8b8g8r8_unorm_pack32";
                  case VK_FORMAT_A8B8G8R8_SNORM_PACK32: return "a8b8g8r8_snorm_pack32";
                  case VK_FORMAT_A8B8G8R8_USCALED_PACK32: return "a8b8g8r8_uscaled_pack32";
                  case VK_FORMAT_A8B8G8R8_SSCALED_PACK32: return "a8b8g8r8_sscaled_pack32";
                  case VK_FORMAT_A8B8G8R8_UINT_PACK32: return "a8b8g8r8_uint_pack32";
                  case VK_FORMAT_A8B8G8R8_SINT_PACK32: return "a8b8g8r8_sint_pack32";
                  case VK_FORMAT_A8B8G8R8_SRGB_PACK32: return "a8b8g8r8_srgb_pack32";
                  case VK_FORMAT_A2R10G10B10_UNORM_PACK32: return "a2r10g10b10_unorm_pack32";
                  case VK_FORMAT_A2R10G10B10_SNORM_PACK32: return "a2r10g10b10_snorm_pack32";
                  case VK_FORMAT_A2R10G10B10_USCALED_PACK32: return "a2r10g10b10_uscaled_pack32";
                  case VK_FORMAT_A2R10G10B10_SSCALED_PACK32: return "a2r10g10b10_sscaled_pack32";
                  case VK_FORMAT_A2R10G10B10_UINT_PACK32: return "a2r10g10b10_uint_pack32";
                  case VK_FORMAT_A2R10G10B10_SINT_PACK32: return "a2r10g10b10_sint_pack32";
                  case VK_FORMAT_A2B10G10R10_UNORM_PACK32: return "a2b10g10r10_unorm_pack32";
                  case VK_FORMAT_A2B10G10R10_SNORM_PACK32: return "a2b10g10r10_snorm_pack32";
                  case VK_FORMAT_A2B10G10R10_USCALED_PACK32: return "a2b10g10r10_uscaled_pack32";
                  case VK_FORMAT_A2B10G10R10_SSCALED_PACK32: return "a2b10g10r10_sscaled_pack32";
                  case VK_FORMAT_A2B10G10R10_UINT_PACK32: return "a2b10g10r10_uint_pack32";
                  case VK_FORMAT_A2B10G10R10_SINT_PACK32: return "a2b10g10r10_sint_pack32";
                  case VK_FORMAT_R16_UNORM: return "r16_unorm";
                  case VK_FORMAT_R16_SNORM: return "r16_snorm";
                  case VK_FORMAT_R16_USCALED: return "r16_uscaled";
                  case VK_FORMAT_R16_SSCALED: return "r16_sscaled";
                  case VK_FORMAT_R16_UINT: return "r16_uint";
                  case VK_FORMAT_R16_SINT: return "r16_sint";
                  case VK_FORMAT_R16_SFLOAT: return "r16_sfloat";
                  case VK_FORMAT_R16G16_UNORM: return "r16g16_unorm";
                  case VK_FORMAT_R16G16_SNORM: return "r16g16_snorm";
                  case VK_FORMAT_R16G16_USCALED: return "r16g16_uscaled";
                  case VK_FORMAT_R16G16_SSCALED: return "r16g16_sscaled";
                  case VK_FORMAT_R16G16_UINT: return "r16g16_uint";
                  case VK_FORMAT_R16G16_SINT: return "r16g16_sint";
                  case VK_FORMAT_R16G16_SFLOAT: return "r16g16_sfloat";
                  case VK_FORMAT_R16G16B16_UNORM: return "r16g16b16_unorm";
                  case VK_FORMAT_R16G16B16_SNORM: return "r16g16b16_snorm";
                  case VK_FORMAT_R16G16B16_USCALED: return "r16g16b16_uscaled";
                  case VK_FORMAT_R16G16B16_SSCALED: return "r16g16b16_sscaled";
                  case VK_FORMAT_R16G16B16_UINT: return "r16g16b16_uint";
                  case VK_FORMAT_R16G16B16_SINT: return "r16g16b16_sint";
                  case VK_FORMAT_R16G16B16_SFLOAT: return "r16g16b16_sfloat";
                  case VK_FORMAT_R16G16B16A16_UNORM: return "r16g16b16a16_unorm";
                  case VK_FORMAT_R16G16B16A16_SNORM: return "r16g16b16a16_snorm";
                  case VK_FORMAT_R16G16B16A16_USCALED: return "r16g16b16a16_uscaled";
                  case VK_FORMAT_R16G16B16A16_SSCALED: return "r16g16b16a16_sscaled";
                  case VK_FORMAT_R16G16B16A16_UINT: return "r16g16b16a16_uint";
                  case VK_FORMAT_R16G16B16A16_SINT: return "r16g16b16a16_sint";
                  case VK_FORMAT_R16G16B16A16_SFLOAT: return "r16g16b16a16_sfloat";
                  case VK_FORMAT_R32_UINT: return "r32_uint";
                  case VK_FORMAT_R32_SINT: return "r32_sint";
                  case VK_FORMAT_R32_SFLOAT: return "r32_sfloat";
                  case VK_FORMAT_R32G32_UINT: return "r32g32_uint";
                  case VK_FORMAT_R32G32_SINT: return "r32g32_sint";
                  case VK_FORMAT_R32G32_SFLOAT: return "r32g32_sfloat";
                  case VK_FORMAT_R32G32B32_UINT: return "r32g32b32_uint";
                  case VK_FORMAT_R32G32B32_SINT: return "r32g32b32_sint";
                  case VK_FORMAT_R32G32B32_SFLOAT: return "r32g32b32_sfloat";
                  case VK_FORMAT_R32G32B32A32_UINT: return "r32g32b32a32_uint";
                  case VK_FORMAT_R32G32B32A32_SINT: return "r32g32b32a32_sint";
                  case VK_FORMAT_R32G32B32A32_SFLOAT: return "r32g32b32a32_sfloat";
                  case VK_FORMAT_R64_UINT: return "r64_uint";
                  case VK_FORMAT_R64_SINT: return "r64_sint";
                  case VK_FORMAT_R64_SFLOAT: return "r64_sfloat";
                  case VK_FORMAT_R64G64_UINT: return "r64g64_uint";
                  case VK_FORMAT_R64G64_SINT: return "r64g64_sint";
                  case VK_FORMAT_R64G64_SFLOAT: return "r64g64_sfloat";
                  case VK_FORMAT_R64G64B64_UINT: return "r64g64b64_uint";
                  case VK_FORMAT_R64G64B64_SINT: return "r64g64b64_sint";
                  case VK_FORMAT_R64G64B64_SFLOAT: return "r64g64b64_sfloat";
                  case VK_FORMAT_R64G64B64A64_UINT: return "r64g64b64a64_uint";
                  case VK_FORMAT_R64G64B64A64_SINT: return "r64g64b64a64_sint";
                  case VK_FORMAT_R64G64B64A64_SFLOAT: return "r64g64b64a64_sfloat";
                  case VK_FORMAT_B10G11R11_UFLOAT_PACK32: return "b10g11r11_ufloat_pack32";
                  case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32: return "e5b9g9r9_ufloat_pack32";
                  case VK_FORMAT_D16_UNORM: return "d16_unorm";
                  case VK_FORMAT_X8_D24_UNORM_PACK32: return "x8_d24_unorm_pack32";
                  case VK_FORMAT_D32_SFLOAT: return "d32_sfloat";
                  case VK_FORMAT_S8_UINT: return "s8_uint";
                  case VK_FORMAT_D16_UNORM_S8_UINT: return "d16_unorm_s8_uint";
                  case VK_FORMAT_D24_UNORM_S8_UINT: return "d24_unorm_s8_uint";
                  case VK_FORMAT_D32_SFLOAT_S8_UINT: return "d32_sfloat_s8_uint";
                  case VK_FORMAT_BC1_RGB_UNORM_BLOCK: return "bc1_rgb_unorm_block";
                  case VK_FORMAT_BC1_RGB_SRGB_BLOCK: return "bc1_rgb_srgb_block";
                  case VK_FORMAT_BC1_RGBA_UNORM_BLOCK: return "bc1_rgba_unorm_block";
                  case VK_FORMAT_BC1_RGBA_SRGB_BLOCK: return "bc1_rgba_srgb_block";
                  case VK_FORMAT_BC2_UNORM_BLOCK: return "bc2_unorm_block";
                  case VK_FORMAT_BC2_SRGB_BLOCK: return "bc2_srgb_block";
                  case VK_FORMAT_BC3_UNORM_BLOCK: return "bc3_unorm_block";
                  case VK_FORMAT_BC3_SRGB_BLOCK: return "bc3_srgb_block";
                  case VK_FORMAT_BC4_UNORM_BLOCK: return "bc4_unorm_block";
                  case VK_FORMAT_BC4_SNORM_BLOCK: return "bc4_snorm_block";
                  case VK_FORMAT_BC5_UNORM_BLOCK: return "bc5_unorm_block";
                  case VK_FORMAT_BC5_SNORM_BLOCK: return "bc5_snorm_block";
                  case VK_FORMAT_BC6H_UFLOAT_BLOCK: return "bc6h_ufloat_block";
                  case VK_FORMAT_BC6H_SFLOAT_BLOCK: return "bc6h_sfloat_block";
                  case VK_FORMAT_BC7_UNORM_BLOCK: return "bc7_unorm_block";
                  case VK_FORMAT_BC7_SRGB_BLOCK: return "bc7_srgb_block";
                  case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK: return "etc2_r8g8b8_unorm_block";
                  case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK: return "etc2_r8g8b8_srgb_block";
                  case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK: return "etc2_r8g8b8a1_unorm_block";
                  case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK: return "etc2_r8g8b8a1_srgb_block";
                  case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK: return "etc2_r8g8b8a8_unorm_block";
                  case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK: return "etc2_r8g8b8a8_srgb_block";
                  case VK_FORMAT_EAC_R11_UNORM_BLOCK: return "eac_r11_unorm_block";
                  case VK_FORMAT_EAC_R11_SNORM_BLOCK: return "eac_r11_snorm_block";
                  case VK_FORMAT_EAC_R11G11_UNORM_BLOCK: return "eac_r11g11_unorm_block";
                  case VK_FORMAT_EAC_R11G11_SNORM_BLOCK: return "eac_r11g11_snorm_block";
                  case VK_FORMAT_ASTC_4x4_UNORM_BLOCK: return "astc_4x4_unorm_block";
                  case VK_FORMAT_ASTC_4x4_SRGB_BLOCK: return "astc_4x4_srgb_block";
                  case VK_FORMAT_ASTC_5x4_UNORM_BLOCK: return "astc_5x4_unorm_block";
                  case VK_FORMAT_ASTC_5x4_SRGB_BLOCK: return "astc_5x4_srgb_block";
                  case VK_FORMAT_ASTC_5x5_UNORM_BLOCK: return "astc_5x5_unorm_block";
                  case VK_FORMAT_ASTC_5x5_SRGB_BLOCK: return "astc_5x5_srgb_block";
                  case VK_FORMAT_ASTC_6x5_UNORM_BLOCK: return "astc_6x5_unorm_block";
                  case VK_FORMAT_ASTC_6x5_SRGB_BLOCK: return "astc_6x5_srgb_block";
                  case VK_FORMAT_ASTC_6x6_UNORM_BLOCK: return "astc_6x6_unorm_block";
                  case VK_FORMAT_ASTC_6x6_SRGB_BLOCK: return "astc_6x6_srgb_block";
                  case VK_FORMAT_ASTC_8x5_UNORM_BLOCK: return "astc_8x5_unorm_block";
                  case VK_FORMAT_ASTC_8x5_SRGB_BLOCK: return "astc_8x5_srgb_block";
                  case VK_FORMAT_ASTC_8x6_UNORM_BLOCK: return "astc_8x6_unorm_block";
                  case VK_FORMAT_ASTC_8x6_SRGB_BLOCK: return "astc_8x6_srgb_block";
                  case VK_FORMAT_ASTC_8x8_UNORM_BLOCK: return "astc_8x8_unorm_block";
                  case VK_FORMAT_ASTC_8x8_SRGB_BLOCK: return "astc_8x8_srgb_block";
                  case VK_FORMAT_ASTC_10x5_UNORM_BLOCK: return "astc_10x5_unorm_block";
                  case VK_FORMAT_ASTC_10x5_SRGB_BLOCK: return "astc_10x5_srgb_block";
                  case VK_FORMAT_ASTC_10x6_UNORM_BLOCK: return "astc_10x6_unorm_block";
                  case VK_FORMAT_ASTC_10x6_SRGB_BLOCK: return "astc_10x6_srgb_block";
                  case VK_FORMAT_ASTC_10x8_UNORM_BLOCK: return "astc_10x8_unorm_block";
                  case VK_FORMAT_ASTC_10x8_SRGB_BLOCK: return "astc_10x8_srgb_block";
                  case VK_FORMAT_ASTC_10x10_UNORM_BLOCK: return "astc_10x10_unorm_block";
                  case VK_FORMAT_ASTC_10x10_SRGB_BLOCK: return "astc_10x10_srgb_block";
                  case VK_FORMAT_ASTC_12x10_UNORM_BLOCK: return "astc_12x10_unorm_block";
                  case VK_FORMAT_ASTC_12x10_SRGB_BLOCK: return "astc_12x10_srgb_block";
                  case VK_FORMAT_ASTC_12x12_UNORM_BLOCK: return "astc_12x12_unorm_block";
                  case VK_FORMAT_ASTC_12x12_SRGB_BLOCK: return "astc_12x12_srgb_block";
                  case VK_FORMAT_G8B8G8R8_422_UNORM: return "g8b8g8r8_422_unorm";
                  case VK_FORMAT_B8G8R8G8_422_UNORM: return "b8g8r8g8_422_unorm";
                  case VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM: return "g8_b8_r8_3plane_420_unorm";
                  case VK_FORMAT_G8_B8R8_2PLANE_420_UNORM: return "g8_b8r8_2plane_420_unorm";
                  case VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM: return "g8_b8_r8_3plane_422_unorm";
                  case VK_FORMAT_G8_B8R8_2PLANE_422_UNORM: return "g8_b8r8_2plane_422_unorm";
                  case VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM: return "g8_b8_r8_3plane_444_unorm";
                  case VK_FORMAT_R10X6_UNORM_PACK16: return "r10x6_unorm_pack16";
                  case VK_FORMAT_R10X6G10X6_UNORM_2PACK16: return "r10x6g10x6_unorm_2pack16";
                  case VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16: return "r10x6g10x6b10x6a10x6_unorm_4pack16";
                  case VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16: return "g10x6b10x6g10x6r10x6_422_unorm_4pack16";
                  case VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16: return "b10x6g10x6r10x6g10x6_422_unorm_4pack16";
                  case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16: return
                              "g10x6_b10x6_r10x6_3plane_420_unorm_3pack16";
                  case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16: return
                              "g10x6_b10x6r10x6_2plane_420_unorm_3pack16";
                  case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16: return
                              "g10x6_b10x6_r10x6_3plane_422_unorm_3pack16";
                  case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16: return
                              "g10x6_b10x6r10x6_2plane_422_unorm_3pack16";
                  case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16: return
                              "g10x6_b10x6_r10x6_3plane_444_unorm_3pack16";
                  case VK_FORMAT_R12X4_UNORM_PACK16: return "r12x4_unorm_pack16";
                  case VK_FORMAT_R12X4G12X4_UNORM_2PACK16: return "r12x4g12x4_unorm_2pack16";
                  case VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16: return "r12x4g12x4b12x4a12x4_unorm_4pack16";
                  case VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16: return "g12x4b12x4g12x4r12x4_422_unorm_4pack16";
                  case VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16: return "b12x4g12x4r12x4g12x4_422_unorm_4pack16";
                  case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16: return
                              "g12x4_b12x4_r12x4_3plane_420_unorm_3pack16";
                  case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16: return
                              "g12x4_b12x4r12x4_2plane_420_unorm_3pack16";
                  case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16: return
                              "g12x4_b12x4_r12x4_3plane_422_unorm_3pack16";
                  case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16: return
                              "g12x4_b12x4r12x4_2plane_422_unorm_3pack16";
                  case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16: return
                              "g12x4_b12x4_r12x4_3plane_444_unorm_3pack16";
                  case VK_FORMAT_G16B16G16R16_422_UNORM: return "g16b16g16r16_422_unorm";
                  case VK_FORMAT_B16G16R16G16_422_UNORM: return "b16g16r16g16_422_unorm";
                  case VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM: return "g16_b16_r16_3plane_420_unorm";
                  case VK_FORMAT_G16_B16R16_2PLANE_420_UNORM: return "g16_b16r16_2plane_420_unorm";
                  case VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM: return "g16_b16_r16_3plane_422_unorm";
                  case VK_FORMAT_G16_B16R16_2PLANE_422_UNORM: return "g16_b16r16_2plane_422_unorm";
                  case VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM: return "g16_b16_r16_3plane_444_unorm";
                  case VK_FORMAT_G8_B8R8_2PLANE_444_UNORM: return "g8_b8r8_2plane_444_unorm";
                  case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16: return
                              "g10x6_b10x6r10x6_2plane_444_unorm_3pack16";
                  case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16: return
                              "g12x4_b12x4r12x4_2plane_444_unorm_3pack16";
                  case VK_FORMAT_G16_B16R16_2PLANE_444_UNORM: return "g16_b16r16_2plane_444_unorm";
                  case VK_FORMAT_A4R4G4B4_UNORM_PACK16: return "a4r4g4b4_unorm_pack16";
                  case VK_FORMAT_A4B4G4R4_UNORM_PACK16: return "a4b4g4r4_unorm_pack16";
                  case VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK: return "astc_4x4_sfloat_block";
                  case VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK: return "astc_5x4_sfloat_block";
                  case VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK: return "astc_5x5_sfloat_block";
                  case VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK: return "astc_6x5_sfloat_block";
                  case VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK: return "astc_6x6_sfloat_block";
                  case VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK: return "astc_8x5_sfloat_block";
                  case VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK: return "astc_8x6_sfloat_block";
                  case VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK: return "astc_8x8_sfloat_block";
                  case VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK: return "astc_10x5_sfloat_block";
                  case VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK: return "astc_10x6_sfloat_block";
                  case VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK: return "astc_10x8_sfloat_block";
                  case VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK: return "astc_10x10_sfloat_block";
                  case VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK: return "astc_12x10_sfloat_block";
                  case VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK: return "astc_12x12_sfloat_block";
                  case VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG: return "pvrtc1_2bpp_unorm_block_img";
                  case VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG: return "pvrtc1_4bpp_unorm_block_img";
                  case VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG: return "pvrtc2_2bpp_unorm_block_img";
                  case VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG: return "pvrtc2_4bpp_unorm_block_img";
                  case VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG: return "pvrtc1_2bpp_srgb_block_img";
                  case VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG: return "pvrtc1_4bpp_srgb_block_img";
                  case VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG: return "pvrtc2_2bpp_srgb_block_img";
                  case VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG: return "pvrtc2_4bpp_srgb_block_img";
                  case VK_FORMAT_R16G16_S10_5_NV: return "r16g16_s10_5_nv";
                  case VK_FORMAT_A1B5G5R5_UNORM_PACK16_KHR: return "a1b5g5r5_unorm_pack16_khr";
                  case VK_FORMAT_A8_UNORM_KHR: return "a8_unorm_khr";
            }
      }

      VkFormat GetVkFormatFromString(const std::string& str) {
            static std::unordered_map<std::string, VkFormat> FormatDic = {
                  {"VK_FORMAT_UNDEFINED", VK_FORMAT_UNDEFINED},
                  {"VK_FORMAT_R4G4_UNORM_PACK8", VK_FORMAT_R4G4_UNORM_PACK8},
                  {"VK_FORMAT_R4G4B4A4_UNORM_PACK16", VK_FORMAT_R4G4B4A4_UNORM_PACK16},
                  {"VK_FORMAT_B4G4R4A4_UNORM_PACK16", VK_FORMAT_B4G4R4A4_UNORM_PACK16},
                  {"VK_FORMAT_R5G6B5_UNORM_PACK16", VK_FORMAT_R5G6B5_UNORM_PACK16},
                  {"VK_FORMAT_B5G6R5_UNORM_PACK16", VK_FORMAT_B5G6R5_UNORM_PACK16},
                  {"VK_FORMAT_R5G5B5A1_UNORM_PACK16", VK_FORMAT_R5G5B5A1_UNORM_PACK16},
                  {"VK_FORMAT_B5G5R5A1_UNORM_PACK16", VK_FORMAT_B5G5R5A1_UNORM_PACK16},
                  {"VK_FORMAT_A1R5G5B5_UNORM_PACK16", VK_FORMAT_A1R5G5B5_UNORM_PACK16},
                  {"VK_FORMAT_R8_UNORM", VK_FORMAT_R8_UNORM},
                  {"VK_FORMAT_R8_SNORM", VK_FORMAT_R8_SNORM},
                  {"VK_FORMAT_R8_USCALED", VK_FORMAT_R8_USCALED},
                  {"VK_FORMAT_R8_SSCALED", VK_FORMAT_R8_SSCALED},
                  {"VK_FORMAT_R8_UINT", VK_FORMAT_R8_UINT},
                  {"VK_FORMAT_R8_SINT", VK_FORMAT_R8_SINT},
                  {"VK_FORMAT_R8_SRGB", VK_FORMAT_R8_SRGB},
                  {"VK_FORMAT_R8G8_UNORM", VK_FORMAT_R8G8_UNORM},
                  {"VK_FORMAT_R8G8_SNORM", VK_FORMAT_R8G8_SNORM},
                  {"VK_FORMAT_R8G8_USCALED", VK_FORMAT_R8G8_USCALED},
                  {"VK_FORMAT_R8G8_SSCALED", VK_FORMAT_R8G8_SSCALED},
                  {"VK_FORMAT_R8G8_UINT", VK_FORMAT_R8G8_UINT},
                  {"VK_FORMAT_R8G8_SINT", VK_FORMAT_R8G8_SINT},
                  {"VK_FORMAT_R8G8_SRGB", VK_FORMAT_R8G8_SRGB},
                  {"VK_FORMAT_R8G8B8_UNORM", VK_FORMAT_R8G8B8_UNORM},
                  {"VK_FORMAT_R8G8B8_SNORM", VK_FORMAT_R8G8B8_SNORM},
                  {"VK_FORMAT_R8G8B8_USCALED", VK_FORMAT_R8G8B8_USCALED},
                  {"VK_FORMAT_R8G8B8_SSCALED", VK_FORMAT_R8G8B8_SSCALED},
                  {"VK_FORMAT_R8G8B8_UINT", VK_FORMAT_R8G8B8_UINT},
                  {"VK_FORMAT_R8G8B8_SINT", VK_FORMAT_R8G8B8_SINT},
                  {"VK_FORMAT_R8G8B8_SRGB", VK_FORMAT_R8G8B8_SRGB},
                  {"VK_FORMAT_B8G8R8_UNORM", VK_FORMAT_B8G8R8_UNORM},
                  {"VK_FORMAT_B8G8R8_SNORM", VK_FORMAT_B8G8R8_SNORM},
                  {"VK_FORMAT_B8G8R8_USCALED", VK_FORMAT_B8G8R8_USCALED},
                  {"VK_FORMAT_B8G8R8_SSCALED", VK_FORMAT_B8G8R8_SSCALED},
                  {"VK_FORMAT_B8G8R8_UINT", VK_FORMAT_B8G8R8_UINT},
                  {"VK_FORMAT_B8G8R8_SINT", VK_FORMAT_B8G8R8_SINT},
                  {"VK_FORMAT_B8G8R8_SRGB", VK_FORMAT_B8G8R8_SRGB},
                  {"VK_FORMAT_R8G8B8A8_UNORM", VK_FORMAT_R8G8B8A8_UNORM},
                  {"VK_FORMAT_R8G8B8A8_SNORM", VK_FORMAT_R8G8B8A8_SNORM},
                  {"VK_FORMAT_R8G8B8A8_USCALED", VK_FORMAT_R8G8B8A8_USCALED},
                  {"VK_FORMAT_R8G8B8A8_SSCALED", VK_FORMAT_R8G8B8A8_SSCALED},
                  {"VK_FORMAT_R8G8B8A8_UINT", VK_FORMAT_R8G8B8A8_UINT},
                  {"VK_FORMAT_R8G8B8A8_SINT", VK_FORMAT_R8G8B8A8_SINT},
                  {"VK_FORMAT_R8G8B8A8_SRGB", VK_FORMAT_R8G8B8A8_SRGB},
                  {"VK_FORMAT_B8G8R8A8_UNORM", VK_FORMAT_B8G8R8A8_UNORM},
                  {"VK_FORMAT_B8G8R8A8_SNORM", VK_FORMAT_B8G8R8A8_SNORM},
                  {"VK_FORMAT_B8G8R8A8_USCALED", VK_FORMAT_B8G8R8A8_USCALED},
                  {"VK_FORMAT_B8G8R8A8_SSCALED", VK_FORMAT_B8G8R8A8_SSCALED},
                  {"VK_FORMAT_B8G8R8A8_UINT", VK_FORMAT_B8G8R8A8_UINT},
                  {"VK_FORMAT_B8G8R8A8_SINT", VK_FORMAT_B8G8R8A8_SINT},
                  {"VK_FORMAT_B8G8R8A8_SRGB", VK_FORMAT_B8G8R8A8_SRGB},
                  {"VK_FORMAT_A8B8G8R8_UNORM_PACK32", VK_FORMAT_A8B8G8R8_UNORM_PACK32},
                  {"VK_FORMAT_A8B8G8R8_SNORM_PACK32", VK_FORMAT_A8B8G8R8_SNORM_PACK32},
                  {"VK_FORMAT_A8B8G8R8_USCALED_PACK32", VK_FORMAT_A8B8G8R8_USCALED_PACK32},
                  {"VK_FORMAT_A8B8G8R8_SSCALED_PACK32", VK_FORMAT_A8B8G8R8_SSCALED_PACK32},
                  {"VK_FORMAT_A8B8G8R8_UINT_PACK32", VK_FORMAT_A8B8G8R8_UINT_PACK32},
                  {"VK_FORMAT_A8B8G8R8_SINT_PACK32", VK_FORMAT_A8B8G8R8_SINT_PACK32},
                  {"VK_FORMAT_A8B8G8R8_SRGB_PACK32", VK_FORMAT_A8B8G8R8_SRGB_PACK32},
                  {"VK_FORMAT_A2R10G10B10_UNORM_PACK32", VK_FORMAT_A2R10G10B10_UNORM_PACK32},
                  {"VK_FORMAT_A2R10G10B10_SNORM_PACK32", VK_FORMAT_A2R10G10B10_SNORM_PACK32},
                  {"VK_FORMAT_A2R10G10B10_USCALED_PACK32", VK_FORMAT_A2R10G10B10_USCALED_PACK32},
                  {"VK_FORMAT_A2R10G10B10_SSCALED_PACK32", VK_FORMAT_A2R10G10B10_SSCALED_PACK32},
                  {"VK_FORMAT_A2R10G10B10_UINT_PACK32", VK_FORMAT_A2R10G10B10_UINT_PACK32},
                  {"VK_FORMAT_A2R10G10B10_SINT_PACK32", VK_FORMAT_A2R10G10B10_SINT_PACK32},
                  {"VK_FORMAT_A2B10G10R10_UNORM_PACK32", VK_FORMAT_A2B10G10R10_UNORM_PACK32},
                  {"VK_FORMAT_A2B10G10R10_SNORM_PACK32", VK_FORMAT_A2B10G10R10_SNORM_PACK32},
                  {"VK_FORMAT_A2B10G10R10_USCALED_PACK32", VK_FORMAT_A2B10G10R10_USCALED_PACK32},
                  {"VK_FORMAT_A2B10G10R10_SSCALED_PACK32", VK_FORMAT_A2B10G10R10_SSCALED_PACK32},
                  {"VK_FORMAT_A2B10G10R10_UINT_PACK32", VK_FORMAT_A2B10G10R10_UINT_PACK32},
                  {"VK_FORMAT_A2B10G10R10_SINT_PACK32", VK_FORMAT_A2B10G10R10_SINT_PACK32},
                  {"VK_FORMAT_R16_UNORM", VK_FORMAT_R16_UNORM},
                  {"VK_FORMAT_R16_SNORM", VK_FORMAT_R16_SNORM},
                  {"VK_FORMAT_R16_USCALED", VK_FORMAT_R16_USCALED},
                  {"VK_FORMAT_R16_SSCALED", VK_FORMAT_R16_SSCALED},
                  {"VK_FORMAT_R16_UINT", VK_FORMAT_R16_UINT},
                  {"VK_FORMAT_R16_SINT", VK_FORMAT_R16_SINT},
                  {"VK_FORMAT_R16_SFLOAT", VK_FORMAT_R16_SFLOAT},
                  {"VK_FORMAT_R16G16_UNORM", VK_FORMAT_R16G16_UNORM},
                  {"VK_FORMAT_R16G16_SNORM", VK_FORMAT_R16G16_SNORM},
                  {"VK_FORMAT_R16G16_USCALED", VK_FORMAT_R16G16_USCALED},
                  {"VK_FORMAT_R16G16_SSCALED", VK_FORMAT_R16G16_SSCALED},
                  {"VK_FORMAT_R16G16_UINT", VK_FORMAT_R16G16_UINT},
                  {"VK_FORMAT_R16G16_SINT", VK_FORMAT_R16G16_SINT},
                  {"VK_FORMAT_R16G16_SFLOAT", VK_FORMAT_R16G16_SFLOAT},
                  {"VK_FORMAT_R16G16B16_UNORM", VK_FORMAT_R16G16B16_UNORM},
                  {"VK_FORMAT_R16G16B16_SNORM", VK_FORMAT_R16G16B16_SNORM},
                  {"VK_FORMAT_R16G16B16_USCALED", VK_FORMAT_R16G16B16_USCALED},
                  {"VK_FORMAT_R16G16B16_SSCALED", VK_FORMAT_R16G16B16_SSCALED},
                  {"VK_FORMAT_R16G16B16_UINT", VK_FORMAT_R16G16B16_UINT},
                  {"VK_FORMAT_R16G16B16_SINT", VK_FORMAT_R16G16B16_SINT},
                  {"VK_FORMAT_R16G16B16_SFLOAT", VK_FORMAT_R16G16B16_SFLOAT},
                  {"VK_FORMAT_R16G16B16A16_UNORM", VK_FORMAT_R16G16B16A16_UNORM},
                  {"VK_FORMAT_R16G16B16A16_SNORM", VK_FORMAT_R16G16B16A16_SNORM},
                  {"VK_FORMAT_R16G16B16A16_USCALED", VK_FORMAT_R16G16B16A16_USCALED},
                  {"VK_FORMAT_R16G16B16A16_SSCALED", VK_FORMAT_R16G16B16A16_SSCALED},
                  {"VK_FORMAT_R16G16B16A16_UINT", VK_FORMAT_R16G16B16A16_UINT},
                  {"VK_FORMAT_R16G16B16A16_SINT", VK_FORMAT_R16G16B16A16_SINT},
                  {"VK_FORMAT_R16G16B16A16_SFLOAT", VK_FORMAT_R16G16B16A16_SFLOAT},
                  {"VK_FORMAT_R32_UINT", VK_FORMAT_R32_UINT},
                  {"VK_FORMAT_R32_SINT", VK_FORMAT_R32_SINT},
                  {"VK_FORMAT_R32_SFLOAT", VK_FORMAT_R32_SFLOAT},
                  {"VK_FORMAT_R32G32_UINT", VK_FORMAT_R32G32_UINT},
                  {"VK_FORMAT_R32G32_SINT", VK_FORMAT_R32G32_SINT},
                  {"VK_FORMAT_R32G32_SFLOAT", VK_FORMAT_R32G32_SFLOAT},
                  {"VK_FORMAT_R32G32B32_UINT", VK_FORMAT_R32G32B32_UINT},
                  {"VK_FORMAT_R32G32B32_SINT", VK_FORMAT_R32G32B32_SINT},
                  {"VK_FORMAT_R32G32B32_SFLOAT", VK_FORMAT_R32G32B32_SFLOAT},
                  {"VK_FORMAT_R32G32B32A32_UINT", VK_FORMAT_R32G32B32A32_UINT},
                  {"VK_FORMAT_R32G32B32A32_SINT", VK_FORMAT_R32G32B32A32_SINT},
                  {"VK_FORMAT_R32G32B32A32_SFLOAT", VK_FORMAT_R32G32B32A32_SFLOAT},
                  {"VK_FORMAT_R64_UINT", VK_FORMAT_R64_UINT},
                  {"VK_FORMAT_R64_SINT", VK_FORMAT_R64_SINT},
                  {"VK_FORMAT_R64_SFLOAT", VK_FORMAT_R64_SFLOAT},
                  {"VK_FORMAT_R64G64_UINT", VK_FORMAT_R64G64_UINT},
                  {"VK_FORMAT_R64G64_SINT", VK_FORMAT_R64G64_SINT},
                  {"VK_FORMAT_R64G64_SFLOAT", VK_FORMAT_R64G64_SFLOAT},
                  {"VK_FORMAT_R64G64B64_UINT", VK_FORMAT_R64G64B64_UINT},
                  {"VK_FORMAT_R64G64B64_SINT", VK_FORMAT_R64G64B64_SINT},
                  {"VK_FORMAT_R64G64B64_SFLOAT", VK_FORMAT_R64G64B64_SFLOAT},
                  {"VK_FORMAT_R64G64B64A64_UINT", VK_FORMAT_R64G64B64A64_UINT},
                  {"VK_FORMAT_R64G64B64A64_SINT", VK_FORMAT_R64G64B64A64_SINT},
                  {"VK_FORMAT_R64G64B64A64_SFLOAT", VK_FORMAT_R64G64B64A64_SFLOAT},
                  {"VK_FORMAT_B10G11R11_UFLOAT_PACK32", VK_FORMAT_B10G11R11_UFLOAT_PACK32},
                  {"VK_FORMAT_E5B9G9R9_UFLOAT_PACK32", VK_FORMAT_E5B9G9R9_UFLOAT_PACK32},
                  {"VK_FORMAT_D16_UNORM", VK_FORMAT_D16_UNORM},
                  {"VK_FORMAT_X8_D24_UNORM_PACK32", VK_FORMAT_X8_D24_UNORM_PACK32},
                  {"VK_FORMAT_D32_SFLOAT", VK_FORMAT_D32_SFLOAT},
                  {"VK_FORMAT_S8_UINT", VK_FORMAT_S8_UINT},
                  {"VK_FORMAT_D16_UNORM_S8_UINT", VK_FORMAT_D16_UNORM_S8_UINT},
                  {"VK_FORMAT_D24_UNORM_S8_UINT", VK_FORMAT_D24_UNORM_S8_UINT},
                  {"VK_FORMAT_D32_SFLOAT_S8_UINT", VK_FORMAT_D32_SFLOAT_S8_UINT},
                  {"VK_FORMAT_BC1_RGB_UNORM_BLOCK", VK_FORMAT_BC1_RGB_UNORM_BLOCK},
                  {"VK_FORMAT_BC1_RGB_SRGB_BLOCK", VK_FORMAT_BC1_RGB_SRGB_BLOCK},
                  {"VK_FORMAT_BC1_RGBA_UNORM_BLOCK", VK_FORMAT_BC1_RGBA_UNORM_BLOCK},
                  {"VK_FORMAT_BC1_RGBA_SRGB_BLOCK", VK_FORMAT_BC1_RGBA_SRGB_BLOCK},
                  {"VK_FORMAT_BC2_UNORM_BLOCK", VK_FORMAT_BC2_UNORM_BLOCK},
                  {"VK_FORMAT_BC2_SRGB_BLOCK", VK_FORMAT_BC2_SRGB_BLOCK},
                  {"VK_FORMAT_BC3_UNORM_BLOCK", VK_FORMAT_BC3_UNORM_BLOCK},
                  {"VK_FORMAT_BC3_SRGB_BLOCK", VK_FORMAT_BC3_SRGB_BLOCK},
                  {"VK_FORMAT_BC4_UNORM_BLOCK", VK_FORMAT_BC4_UNORM_BLOCK},
                  {"VK_FORMAT_BC4_SNORM_BLOCK", VK_FORMAT_BC4_SNORM_BLOCK},
                  {"VK_FORMAT_BC5_UNORM_BLOCK", VK_FORMAT_BC5_UNORM_BLOCK},
                  {"VK_FORMAT_BC5_SNORM_BLOCK", VK_FORMAT_BC5_SNORM_BLOCK},
                  {"VK_FORMAT_BC6H_UFLOAT_BLOCK", VK_FORMAT_BC6H_UFLOAT_BLOCK},
                  {"VK_FORMAT_BC6H_SFLOAT_BLOCK", VK_FORMAT_BC6H_SFLOAT_BLOCK},
                  {"VK_FORMAT_BC7_UNORM_BLOCK", VK_FORMAT_BC7_UNORM_BLOCK},
                  {"VK_FORMAT_BC7_SRGB_BLOCK", VK_FORMAT_BC7_SRGB_BLOCK},
                  {"VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK", VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK},
                  {"VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK", VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK},
                  {"VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK", VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK},
                  {"VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK", VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK},
                  {"VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK", VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK},
                  {"VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK", VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK},
                  {"VK_FORMAT_EAC_R11_UNORM_BLOCK", VK_FORMAT_EAC_R11_UNORM_BLOCK},
                  {"VK_FORMAT_EAC_R11_SNORM_BLOCK", VK_FORMAT_EAC_R11_SNORM_BLOCK},
                  {"VK_FORMAT_EAC_R11G11_UNORM_BLOCK", VK_FORMAT_EAC_R11G11_UNORM_BLOCK},
                  {"VK_FORMAT_EAC_R11G11_SNORM_BLOCK", VK_FORMAT_EAC_R11G11_SNORM_BLOCK},
                  {"VK_FORMAT_ASTC_4x4_UNORM_BLOCK", VK_FORMAT_ASTC_4x4_UNORM_BLOCK},
                  {"VK_FORMAT_ASTC_4x4_SRGB_BLOCK", VK_FORMAT_ASTC_4x4_SRGB_BLOCK},
                  {"VK_FORMAT_ASTC_5x4_UNORM_BLOCK", VK_FORMAT_ASTC_5x4_UNORM_BLOCK},
                  {"VK_FORMAT_ASTC_5x4_SRGB_BLOCK", VK_FORMAT_ASTC_5x4_SRGB_BLOCK},
                  {"VK_FORMAT_ASTC_5x5_UNORM_BLOCK", VK_FORMAT_ASTC_5x5_UNORM_BLOCK},
                  {"VK_FORMAT_ASTC_5x5_SRGB_BLOCK", VK_FORMAT_ASTC_5x5_SRGB_BLOCK},
                  {"VK_FORMAT_ASTC_6x5_UNORM_BLOCK", VK_FORMAT_ASTC_6x5_UNORM_BLOCK},
                  {"VK_FORMAT_ASTC_6x5_SRGB_BLOCK", VK_FORMAT_ASTC_6x5_SRGB_BLOCK},
                  {"VK_FORMAT_ASTC_6x6_UNORM_BLOCK", VK_FORMAT_ASTC_6x6_UNORM_BLOCK},
                  {"VK_FORMAT_ASTC_6x6_SRGB_BLOCK", VK_FORMAT_ASTC_6x6_SRGB_BLOCK},
                  {"VK_FORMAT_ASTC_8x5_UNORM_BLOCK", VK_FORMAT_ASTC_8x5_UNORM_BLOCK},
                  {"VK_FORMAT_ASTC_8x5_SRGB_BLOCK", VK_FORMAT_ASTC_8x5_SRGB_BLOCK},
                  {"VK_FORMAT_ASTC_8x6_UNORM_BLOCK", VK_FORMAT_ASTC_8x6_UNORM_BLOCK},
                  {"VK_FORMAT_ASTC_8x6_SRGB_BLOCK", VK_FORMAT_ASTC_8x6_SRGB_BLOCK},
                  {"VK_FORMAT_ASTC_8x8_UNORM_BLOCK", VK_FORMAT_ASTC_8x8_UNORM_BLOCK},
                  {"VK_FORMAT_ASTC_8x8_SRGB_BLOCK", VK_FORMAT_ASTC_8x8_SRGB_BLOCK},
                  {"VK_FORMAT_ASTC_10x5_UNORM_BLOCK", VK_FORMAT_ASTC_10x5_UNORM_BLOCK},
                  {"VK_FORMAT_ASTC_10x5_SRGB_BLOCK", VK_FORMAT_ASTC_10x5_SRGB_BLOCK},
                  {"VK_FORMAT_ASTC_10x6_UNORM_BLOCK", VK_FORMAT_ASTC_10x6_UNORM_BLOCK},
                  {"VK_FORMAT_ASTC_10x6_SRGB_BLOCK", VK_FORMAT_ASTC_10x6_SRGB_BLOCK},
                  {"VK_FORMAT_ASTC_10x8_UNORM_BLOCK", VK_FORMAT_ASTC_10x8_UNORM_BLOCK},
                  {"VK_FORMAT_ASTC_10x8_SRGB_BLOCK", VK_FORMAT_ASTC_10x8_SRGB_BLOCK},
                  {"VK_FORMAT_ASTC_10x10_UNORM_BLOCK", VK_FORMAT_ASTC_10x10_UNORM_BLOCK},
                  {"VK_FORMAT_ASTC_10x10_SRGB_BLOCK", VK_FORMAT_ASTC_10x10_SRGB_BLOCK},
                  {"VK_FORMAT_ASTC_12x10_UNORM_BLOCK", VK_FORMAT_ASTC_12x10_UNORM_BLOCK},
                  {"VK_FORMAT_ASTC_12x10_SRGB_BLOCK", VK_FORMAT_ASTC_12x10_SRGB_BLOCK},
                  {"VK_FORMAT_ASTC_12x12_UNORM_BLOCK", VK_FORMAT_ASTC_12x12_UNORM_BLOCK},
                  {"VK_FORMAT_ASTC_12x12_SRGB_BLOCK", VK_FORMAT_ASTC_12x12_SRGB_BLOCK},
                  {"VK_FORMAT_G8B8G8R8_422_UNORM", VK_FORMAT_G8B8G8R8_422_UNORM},
                  {"VK_FORMAT_B8G8R8G8_422_UNORM", VK_FORMAT_B8G8R8G8_422_UNORM},
                  {"VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM", VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM},
                  {"VK_FORMAT_G8_B8R8_2PLANE_420_UNORM", VK_FORMAT_G8_B8R8_2PLANE_420_UNORM},
                  {"VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM", VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM},
                  {"VK_FORMAT_G8_B8R8_2PLANE_422_UNORM", VK_FORMAT_G8_B8R8_2PLANE_422_UNORM},
                  {"VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM", VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM},
                  {"VK_FORMAT_R10X6_UNORM_PACK16", VK_FORMAT_R10X6_UNORM_PACK16},
                  {"VK_FORMAT_R10X6G10X6_UNORM_2PACK16", VK_FORMAT_R10X6G10X6_UNORM_2PACK16},
                  {"VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16", VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16},
                  {"VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16", VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16},
                  {"VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16", VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16},
                  {"VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16", VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16},
                  {"VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16", VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16},
                  {"VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16", VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16},
                  {"VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16", VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16},
                  {"VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16", VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16},
                  {"VK_FORMAT_R12X4_UNORM_PACK16", VK_FORMAT_R12X4_UNORM_PACK16},
                  {"VK_FORMAT_R12X4G12X4_UNORM_2PACK16", VK_FORMAT_R12X4G12X4_UNORM_2PACK16},
                  {"VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16", VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16},
                  {"VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16", VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16},
                  {"VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16", VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16},
                  {"VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16", VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16},
                  {"VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16", VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16},
                  {"VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16", VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16},
                  {"VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16", VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16},
                  {"VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16", VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16},
                  {"VK_FORMAT_G16B16G16R16_422_UNORM", VK_FORMAT_G16B16G16R16_422_UNORM},
                  {"VK_FORMAT_B16G16R16G16_422_UNORM", VK_FORMAT_B16G16R16G16_422_UNORM},
                  {"VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM", VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM},
                  {"VK_FORMAT_G16_B16R16_2PLANE_420_UNORM", VK_FORMAT_G16_B16R16_2PLANE_420_UNORM},
                  {"VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM", VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM},
                  {"VK_FORMAT_G16_B16R16_2PLANE_422_UNORM", VK_FORMAT_G16_B16R16_2PLANE_422_UNORM},
                  {"VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM", VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM},
                  {"VK_FORMAT_G8_B8R8_2PLANE_444_UNORM", VK_FORMAT_G8_B8R8_2PLANE_444_UNORM},
                  {"VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16", VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16},
                  {"VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16", VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16},
                  {"VK_FORMAT_G16_B16R16_2PLANE_444_UNORM", VK_FORMAT_G16_B16R16_2PLANE_444_UNORM},
                  {"VK_FORMAT_A4R4G4B4_UNORM_PACK16", VK_FORMAT_A4R4G4B4_UNORM_PACK16},
                  {"VK_FORMAT_A4B4G4R4_UNORM_PACK16", VK_FORMAT_A4B4G4R4_UNORM_PACK16},
                  {"VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK", VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK},
                  {"VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK", VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK},
                  {"VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK", VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK},
                  {"VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK", VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK},
                  {"VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK", VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK},
                  {"VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK", VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK},
                  {"VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK", VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK},
                  {"VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK", VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK},
                  {"VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK", VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK},
                  {"VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK", VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK},
                  {"VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK", VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK},
                  {"VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK", VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK},
                  {"VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK", VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK},
                  {"VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK", VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK},
                  {"VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG", VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG},
                  {"VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG", VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG},
                  {"VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG", VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG},
                  {"VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG", VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG},
                  {"VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG", VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG},
                  {"VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG", VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG},
                  {"VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG", VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG},
                  {"VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG", VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG},
                  {"VK_FORMAT_R16G16_S10_5_NV", VK_FORMAT_R16G16_S10_5_NV},
                  {"VK_FORMAT_A1B5G5R5_UNORM_PACK16_KHR", VK_FORMAT_A1B5G5R5_UNORM_PACK16_KHR},
                  {"VK_FORMAT_A8_UNORM_KHR", VK_FORMAT_A8_UNORM_KHR},
            };

            if (const auto res = FormatDic.find(str); res != FormatDic.end()) {
                  return res->second;
            } else {
                  return VK_FORMAT_UNDEFINED;
            }
      }

      VkFormat GetVkFormatFromStringSimpled(const std::string& str) {
            static std::unordered_map<std::string, VkFormat> FormatDic = {
                  {"undefined", VK_FORMAT_UNDEFINED},
                  {"r4g4_unorm_pack8", VK_FORMAT_R4G4_UNORM_PACK8},
                  {"r4g4b4a4_unorm_pack16", VK_FORMAT_R4G4B4A4_UNORM_PACK16},
                  {"b4g4r4a4_unorm_pack16", VK_FORMAT_B4G4R4A4_UNORM_PACK16},
                  {"r5g6b5_unorm_pack16", VK_FORMAT_R5G6B5_UNORM_PACK16},
                  {"b5g6r5_unorm_pack16", VK_FORMAT_B5G6R5_UNORM_PACK16},
                  {"r5g5b5a1_unorm_pack16", VK_FORMAT_R5G5B5A1_UNORM_PACK16},
                  {"b5g5r5a1_unorm_pack16", VK_FORMAT_B5G5R5A1_UNORM_PACK16},
                  {"a1r5g5b5_unorm_pack16", VK_FORMAT_A1R5G5B5_UNORM_PACK16},
                  {"r8_unorm", VK_FORMAT_R8_UNORM},
                  {"r8_snorm", VK_FORMAT_R8_SNORM},
                  {"r8_uscaled", VK_FORMAT_R8_USCALED},
                  {"r8_sscaled", VK_FORMAT_R8_SSCALED},
                  {"r8_uint", VK_FORMAT_R8_UINT},
                  {"r8_sint", VK_FORMAT_R8_SINT},
                  {"r8_srgb", VK_FORMAT_R8_SRGB},
                  {"r8g8_unorm", VK_FORMAT_R8G8_UNORM},
                  {"r8g8_snorm", VK_FORMAT_R8G8_SNORM},
                  {"r8g8_uscaled", VK_FORMAT_R8G8_USCALED},
                  {"r8g8_sscaled", VK_FORMAT_R8G8_SSCALED},
                  {"r8g8_uint", VK_FORMAT_R8G8_UINT},
                  {"r8g8_sint", VK_FORMAT_R8G8_SINT},
                  {"r8g8_srgb", VK_FORMAT_R8G8_SRGB},
                  {"r8g8b8_unorm", VK_FORMAT_R8G8B8_UNORM},
                  {"r8g8b8_snorm", VK_FORMAT_R8G8B8_SNORM},
                  {"r8g8b8_uscaled", VK_FORMAT_R8G8B8_USCALED},
                  {"r8g8b8_sscaled", VK_FORMAT_R8G8B8_SSCALED},
                  {"r8g8b8_uint", VK_FORMAT_R8G8B8_UINT},
                  {"r8g8b8_sint", VK_FORMAT_R8G8B8_SINT},
                  {"r8g8b8_srgb", VK_FORMAT_R8G8B8_SRGB},
                  {"b8g8r8_unorm", VK_FORMAT_B8G8R8_UNORM},
                  {"b8g8r8_snorm", VK_FORMAT_B8G8R8_SNORM},
                  {"b8g8r8_uscaled", VK_FORMAT_B8G8R8_USCALED},
                  {"b8g8r8_sscaled", VK_FORMAT_B8G8R8_SSCALED},
                  {"b8g8r8_uint", VK_FORMAT_B8G8R8_UINT},
                  {"b8g8r8_sint", VK_FORMAT_B8G8R8_SINT},
                  {"b8g8r8_srgb", VK_FORMAT_B8G8R8_SRGB},
                  {"r8g8b8a8_unorm", VK_FORMAT_R8G8B8A8_UNORM},
                  {"r8g8b8a8_snorm", VK_FORMAT_R8G8B8A8_SNORM},
                  {"r8g8b8a8_uscaled", VK_FORMAT_R8G8B8A8_USCALED},
                  {"r8g8b8a8_sscaled", VK_FORMAT_R8G8B8A8_SSCALED},
                  {"r8g8b8a8_uint", VK_FORMAT_R8G8B8A8_UINT},
                  {"r8g8b8a8_sint", VK_FORMAT_R8G8B8A8_SINT},
                  {"r8g8b8a8_srgb", VK_FORMAT_R8G8B8A8_SRGB},
                  {"b8g8r8a8_unorm", VK_FORMAT_B8G8R8A8_UNORM},
                  {"b8g8r8a8_snorm", VK_FORMAT_B8G8R8A8_SNORM},
                  {"b8g8r8a8_uscaled", VK_FORMAT_B8G8R8A8_USCALED},
                  {"b8g8r8a8_sscaled", VK_FORMAT_B8G8R8A8_SSCALED},
                  {"b8g8r8a8_uint", VK_FORMAT_B8G8R8A8_UINT},
                  {"b8g8r8a8_sint", VK_FORMAT_B8G8R8A8_SINT},
                  {"b8g8r8a8_srgb", VK_FORMAT_B8G8R8A8_SRGB},
                  {"a8b8g8r8_unorm_pack32", VK_FORMAT_A8B8G8R8_UNORM_PACK32},
                  {"a8b8g8r8_snorm_pack32", VK_FORMAT_A8B8G8R8_SNORM_PACK32},
                  {"a8b8g8r8_uscaled_pack32", VK_FORMAT_A8B8G8R8_USCALED_PACK32},
                  {"a8b8g8r8_sscaled_pack32", VK_FORMAT_A8B8G8R8_SSCALED_PACK32},
                  {"a8b8g8r8_uint_pack32", VK_FORMAT_A8B8G8R8_UINT_PACK32},
                  {"a8b8g8r8_sint_pack32", VK_FORMAT_A8B8G8R8_SINT_PACK32},
                  {"a8b8g8r8_srgb_pack32", VK_FORMAT_A8B8G8R8_SRGB_PACK32},
                  {"a2r10g10b10_unorm_pack32", VK_FORMAT_A2R10G10B10_UNORM_PACK32},
                  {"a2r10g10b10_snorm_pack32", VK_FORMAT_A2R10G10B10_SNORM_PACK32},
                  {"a2r10g10b10_uscaled_pack32", VK_FORMAT_A2R10G10B10_USCALED_PACK32},
                  {"a2r10g10b10_sscaled_pack32", VK_FORMAT_A2R10G10B10_SSCALED_PACK32},
                  {"a2r10g10b10_uint_pack32", VK_FORMAT_A2R10G10B10_UINT_PACK32},
                  {"a2r10g10b10_sint_pack32", VK_FORMAT_A2R10G10B10_SINT_PACK32},
                  {"a2b10g10r10_unorm_pack32", VK_FORMAT_A2B10G10R10_UNORM_PACK32},
                  {"a2b10g10r10_snorm_pack32", VK_FORMAT_A2B10G10R10_SNORM_PACK32},
                  {"a2b10g10r10_uscaled_pack32", VK_FORMAT_A2B10G10R10_USCALED_PACK32},
                  {"a2b10g10r10_sscaled_pack32", VK_FORMAT_A2B10G10R10_SSCALED_PACK32},
                  {"a2b10g10r10_uint_pack32", VK_FORMAT_A2B10G10R10_UINT_PACK32},
                  {"a2b10g10r10_sint_pack32", VK_FORMAT_A2B10G10R10_SINT_PACK32},
                  {"r16_unorm", VK_FORMAT_R16_UNORM},
                  {"r16_snorm", VK_FORMAT_R16_SNORM},
                  {"r16_uscaled", VK_FORMAT_R16_USCALED},
                  {"r16_sscaled", VK_FORMAT_R16_SSCALED},
                  {"r16_uint", VK_FORMAT_R16_UINT},
                  {"r16_sint", VK_FORMAT_R16_SINT},
                  {"r16_sfloat", VK_FORMAT_R16_SFLOAT},
                  {"r16g16_unorm", VK_FORMAT_R16G16_UNORM},
                  {"r16g16_snorm", VK_FORMAT_R16G16_SNORM},
                  {"r16g16_uscaled", VK_FORMAT_R16G16_USCALED},
                  {"r16g16_sscaled", VK_FORMAT_R16G16_SSCALED},
                  {"r16g16_uint", VK_FORMAT_R16G16_UINT},
                  {"r16g16_sint", VK_FORMAT_R16G16_SINT},
                  {"r16g16_sfloat", VK_FORMAT_R16G16_SFLOAT},
                  {"r16g16b16_unorm", VK_FORMAT_R16G16B16_UNORM},
                  {"r16g16b16_snorm", VK_FORMAT_R16G16B16_SNORM},
                  {"r16g16b16_uscaled", VK_FORMAT_R16G16B16_USCALED},
                  {"r16g16b16_sscaled", VK_FORMAT_R16G16B16_SSCALED},
                  {"r16g16b16_uint", VK_FORMAT_R16G16B16_UINT},
                  {"r16g16b16_sint", VK_FORMAT_R16G16B16_SINT},
                  {"r16g16b16_sfloat", VK_FORMAT_R16G16B16_SFLOAT},
                  {"r16g16b16a16_unorm", VK_FORMAT_R16G16B16A16_UNORM},
                  {"r16g16b16a16_snorm", VK_FORMAT_R16G16B16A16_SNORM},
                  {"r16g16b16a16_uscaled", VK_FORMAT_R16G16B16A16_USCALED},
                  {"r16g16b16a16_sscaled", VK_FORMAT_R16G16B16A16_SSCALED},
                  {"r16g16b16a16_uint", VK_FORMAT_R16G16B16A16_UINT},
                  {"r16g16b16a16_sint", VK_FORMAT_R16G16B16A16_SINT},
                  {"r16g16b16a16_sfloat", VK_FORMAT_R16G16B16A16_SFLOAT},
                  {"r32_uint", VK_FORMAT_R32_UINT},
                  {"r32_sint", VK_FORMAT_R32_SINT},
                  {"r32_sfloat", VK_FORMAT_R32_SFLOAT},
                  {"r32g32_uint", VK_FORMAT_R32G32_UINT},
                  {"r32g32_sint", VK_FORMAT_R32G32_SINT},
                  {"r32g32_sfloat", VK_FORMAT_R32G32_SFLOAT},
                  {"r32g32b32_uint", VK_FORMAT_R32G32B32_UINT},
                  {"r32g32b32_sint", VK_FORMAT_R32G32B32_SINT},
                  {"r32g32b32_sfloat", VK_FORMAT_R32G32B32_SFLOAT},
                  {"r32g32b32a32_uint", VK_FORMAT_R32G32B32A32_UINT},
                  {"r32g32b32a32_sint", VK_FORMAT_R32G32B32A32_SINT},
                  {"r32g32b32a32_sfloat", VK_FORMAT_R32G32B32A32_SFLOAT},
                  {"r64_uint", VK_FORMAT_R64_UINT},
                  {"r64_sint", VK_FORMAT_R64_SINT},
                  {"r64_sfloat", VK_FORMAT_R64_SFLOAT},
                  {"r64g64_uint", VK_FORMAT_R64G64_UINT},
                  {"r64g64_sint", VK_FORMAT_R64G64_SINT},
                  {"r64g64_sfloat", VK_FORMAT_R64G64_SFLOAT},
                  {"r64g64b64_uint", VK_FORMAT_R64G64B64_UINT},
                  {"r64g64b64_sint", VK_FORMAT_R64G64B64_SINT},
                  {"r64g64b64_sfloat", VK_FORMAT_R64G64B64_SFLOAT},
                  {"r64g64b64a64_uint", VK_FORMAT_R64G64B64A64_UINT},
                  {"r64g64b64a64_sint", VK_FORMAT_R64G64B64A64_SINT},
                  {"r64g64b64a64_sfloat", VK_FORMAT_R64G64B64A64_SFLOAT},
                  {"b10g11r11_ufloat_pack32", VK_FORMAT_B10G11R11_UFLOAT_PACK32},
                  {"e5b9g9r9_ufloat_pack32", VK_FORMAT_E5B9G9R9_UFLOAT_PACK32},
                  {"d16_unorm", VK_FORMAT_D16_UNORM},
                  {"x8_d24_unorm_pack32", VK_FORMAT_X8_D24_UNORM_PACK32},
                  {"d32_sfloat", VK_FORMAT_D32_SFLOAT},
                  {"s8_uint", VK_FORMAT_S8_UINT},
                  {"d16_unorm_s8_uint", VK_FORMAT_D16_UNORM_S8_UINT},
                  {"d24_unorm_s8_uint", VK_FORMAT_D24_UNORM_S8_UINT},
                  {"d32_sfloat_s8_uint", VK_FORMAT_D32_SFLOAT_S8_UINT},
                  {"bc1_rgb_unorm_block", VK_FORMAT_BC1_RGB_UNORM_BLOCK},
                  {"bc1_rgb_srgb_block", VK_FORMAT_BC1_RGB_SRGB_BLOCK},
                  {"bc1_rgba_unorm_block", VK_FORMAT_BC1_RGBA_UNORM_BLOCK},
                  {"bc1_rgba_srgb_block", VK_FORMAT_BC1_RGBA_SRGB_BLOCK},
                  {"bc2_unorm_block", VK_FORMAT_BC2_UNORM_BLOCK},
                  {"bc2_srgb_block", VK_FORMAT_BC2_SRGB_BLOCK},
                  {"bc3_unorm_block", VK_FORMAT_BC3_UNORM_BLOCK},
                  {"bc3_srgb_block", VK_FORMAT_BC3_SRGB_BLOCK},
                  {"bc4_unorm_block", VK_FORMAT_BC4_UNORM_BLOCK},
                  {"bc4_snorm_block", VK_FORMAT_BC4_SNORM_BLOCK},
                  {"bc5_unorm_block", VK_FORMAT_BC5_UNORM_BLOCK},
                  {"bc5_snorm_block", VK_FORMAT_BC5_SNORM_BLOCK},
                  {"bc6h_ufloat_block", VK_FORMAT_BC6H_UFLOAT_BLOCK},
                  {"bc6h_sfloat_block", VK_FORMAT_BC6H_SFLOAT_BLOCK},
                  {"bc7_unorm_block", VK_FORMAT_BC7_UNORM_BLOCK},
                  {"bc7_srgb_block", VK_FORMAT_BC7_SRGB_BLOCK},
                  {"etc2_r8g8b8_unorm_block", VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK},
                  {"etc2_r8g8b8_srgb_block", VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK},
                  {"etc2_r8g8b8a1_unorm_block", VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK},
                  {"etc2_r8g8b8a1_srgb_block", VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK},
                  {"etc2_r8g8b8a8_unorm_block", VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK},
                  {"etc2_r8g8b8a8_srgb_block", VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK},
                  {"eac_r11_unorm_block", VK_FORMAT_EAC_R11_UNORM_BLOCK},
                  {"eac_r11_snorm_block", VK_FORMAT_EAC_R11_SNORM_BLOCK},
                  {"eac_r11g11_unorm_block", VK_FORMAT_EAC_R11G11_UNORM_BLOCK},
                  {"eac_r11g11_snorm_block", VK_FORMAT_EAC_R11G11_SNORM_BLOCK},
                  {"astc_4x4_unorm_block", VK_FORMAT_ASTC_4x4_UNORM_BLOCK},
                  {"astc_4x4_srgb_block", VK_FORMAT_ASTC_4x4_SRGB_BLOCK},
                  {"astc_5x4_unorm_block", VK_FORMAT_ASTC_5x4_UNORM_BLOCK},
                  {"astc_5x4_srgb_block", VK_FORMAT_ASTC_5x4_SRGB_BLOCK},
                  {"astc_5x5_unorm_block", VK_FORMAT_ASTC_5x5_UNORM_BLOCK},
                  {"astc_5x5_srgb_block", VK_FORMAT_ASTC_5x5_SRGB_BLOCK},
                  {"astc_6x5_unorm_block", VK_FORMAT_ASTC_6x5_UNORM_BLOCK},
                  {"astc_6x5_srgb_block", VK_FORMAT_ASTC_6x5_SRGB_BLOCK},
                  {"astc_6x6_unorm_block", VK_FORMAT_ASTC_6x6_UNORM_BLOCK},
                  {"astc_6x6_srgb_block", VK_FORMAT_ASTC_6x6_SRGB_BLOCK},
                  {"astc_8x5_unorm_block", VK_FORMAT_ASTC_8x5_UNORM_BLOCK},
                  {"astc_8x5_srgb_block", VK_FORMAT_ASTC_8x5_SRGB_BLOCK},
                  {"astc_8x6_unorm_block", VK_FORMAT_ASTC_8x6_UNORM_BLOCK},
                  {"astc_8x6_srgb_block", VK_FORMAT_ASTC_8x6_SRGB_BLOCK},
                  {"astc_8x8_unorm_block", VK_FORMAT_ASTC_8x8_UNORM_BLOCK},
                  {"astc_8x8_srgb_block", VK_FORMAT_ASTC_8x8_SRGB_BLOCK},
                  {"astc_10x5_unorm_block", VK_FORMAT_ASTC_10x5_UNORM_BLOCK},
                  {"astc_10x5_srgb_block", VK_FORMAT_ASTC_10x5_SRGB_BLOCK},
                  {"astc_10x6_unorm_block", VK_FORMAT_ASTC_10x6_UNORM_BLOCK},
                  {"astc_10x6_srgb_block", VK_FORMAT_ASTC_10x6_SRGB_BLOCK},
                  {"astc_10x8_unorm_block", VK_FORMAT_ASTC_10x8_UNORM_BLOCK},
                  {"astc_10x8_srgb_block", VK_FORMAT_ASTC_10x8_SRGB_BLOCK},
                  {"astc_10x10_unorm_block", VK_FORMAT_ASTC_10x10_UNORM_BLOCK},
                  {"astc_10x10_srgb_block", VK_FORMAT_ASTC_10x10_SRGB_BLOCK},
                  {"astc_12x10_unorm_block", VK_FORMAT_ASTC_12x10_UNORM_BLOCK},
                  {"astc_12x10_srgb_block", VK_FORMAT_ASTC_12x10_SRGB_BLOCK},
                  {"astc_12x12_unorm_block", VK_FORMAT_ASTC_12x12_UNORM_BLOCK},
                  {"astc_12x12_srgb_block", VK_FORMAT_ASTC_12x12_SRGB_BLOCK},
                  {"g8b8g8r8_422_unorm", VK_FORMAT_G8B8G8R8_422_UNORM},
                  {"b8g8r8g8_422_unorm", VK_FORMAT_B8G8R8G8_422_UNORM},
                  {"g8_b8_r8_3plane_420_unorm", VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM},
                  {"g8_b8r8_2plane_420_unorm", VK_FORMAT_G8_B8R8_2PLANE_420_UNORM},
                  {"g8_b8_r8_3plane_422_unorm", VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM},
                  {"g8_b8r8_2plane_422_unorm", VK_FORMAT_G8_B8R8_2PLANE_422_UNORM},
                  {"g8_b8_r8_3plane_444_unorm", VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM},
                  {"r10x6_unorm_pack16", VK_FORMAT_R10X6_UNORM_PACK16},
                  {"r10x6g10x6_unorm_2pack16", VK_FORMAT_R10X6G10X6_UNORM_2PACK16},
                  {"r10x6g10x6b10x6a10x6_unorm_4pack16", VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16},
                  {"g10x6b10x6g10x6r10x6_422_unorm_4pack16", VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16},
                  {"b10x6g10x6r10x6g10x6_422_unorm_4pack16", VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16},
                  {"g10x6_b10x6_r10x6_3plane_420_unorm_3pack16", VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16},
                  {"g10x6_b10x6r10x6_2plane_420_unorm_3pack16", VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16},
                  {"g10x6_b10x6_r10x6_3plane_422_unorm_3pack16", VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16},
                  {"g10x6_b10x6r10x6_2plane_422_unorm_3pack16", VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16},
                  {"g10x6_b10x6_r10x6_3plane_444_unorm_3pack16", VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16},
                  {"r12x4_unorm_pack16", VK_FORMAT_R12X4_UNORM_PACK16},
                  {"r12x4g12x4_unorm_2pack16", VK_FORMAT_R12X4G12X4_UNORM_2PACK16},
                  {"r12x4g12x4b12x4a12x4_unorm_4pack16", VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16},
                  {"g12x4b12x4g12x4r12x4_422_unorm_4pack16", VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16},
                  {"b12x4g12x4r12x4g12x4_422_unorm_4pack16", VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16},
                  {"g12x4_b12x4_r12x4_3plane_420_unorm_3pack16", VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16},
                  {"g12x4_b12x4r12x4_2plane_420_unorm_3pack16", VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16},
                  {"g12x4_b12x4_r12x4_3plane_422_unorm_3pack16", VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16},
                  {"g12x4_b12x4r12x4_2plane_422_unorm_3pack16", VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16},
                  {"g12x4_b12x4_r12x4_3plane_444_unorm_3pack16", VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16},
                  {"g16b16g16r16_422_unorm", VK_FORMAT_G16B16G16R16_422_UNORM},
                  {"b16g16r16g16_422_unorm", VK_FORMAT_B16G16R16G16_422_UNORM},
                  {"g16_b16_r16_3plane_420_unorm", VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM},
                  {"g16_b16r16_2plane_420_unorm", VK_FORMAT_G16_B16R16_2PLANE_420_UNORM},
                  {"g16_b16_r16_3plane_422_unorm", VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM},
                  {"g16_b16r16_2plane_422_unorm", VK_FORMAT_G16_B16R16_2PLANE_422_UNORM},
                  {"g16_b16_r16_3plane_444_unorm", VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM},
                  {"g8_b8r8_2plane_444_unorm", VK_FORMAT_G8_B8R8_2PLANE_444_UNORM},
                  {"g10x6_b10x6r10x6_2plane_444_unorm_3pack16", VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16},
                  {"g12x4_b12x4r12x4_2plane_444_unorm_3pack16", VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16},
                  {"g16_b16r16_2plane_444_unorm", VK_FORMAT_G16_B16R16_2PLANE_444_UNORM},
                  {"a4r4g4b4_unorm_pack16", VK_FORMAT_A4R4G4B4_UNORM_PACK16},
                  {"a4b4g4r4_unorm_pack16", VK_FORMAT_A4B4G4R4_UNORM_PACK16},
                  {"astc_4x4_sfloat_block", VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK},
                  {"astc_5x4_sfloat_block", VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK},
                  {"astc_5x5_sfloat_block", VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK},
                  {"astc_6x5_sfloat_block", VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK},
                  {"astc_6x6_sfloat_block", VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK},
                  {"astc_8x5_sfloat_block", VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK},
                  {"astc_8x6_sfloat_block", VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK},
                  {"astc_8x8_sfloat_block", VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK},
                  {"astc_10x5_sfloat_block", VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK},
                  {"astc_10x6_sfloat_block", VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK},
                  {"astc_10x8_sfloat_block", VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK},
                  {"astc_10x10_sfloat_block", VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK},
                  {"astc_12x10_sfloat_block", VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK},
                  {"astc_12x12_sfloat_block", VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK},
                  {"pvrtc1_2bpp_unorm_block_img", VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG},
                  {"pvrtc1_4bpp_unorm_block_img", VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG},
                  {"pvrtc2_2bpp_unorm_block_img", VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG},
                  {"pvrtc2_4bpp_unorm_block_img", VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG},
                  {"pvrtc1_2bpp_srgb_block_img", VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG},
                  {"pvrtc1_4bpp_srgb_block_img", VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG},
                  {"pvrtc2_2bpp_srgb_block_img", VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG},
                  {"pvrtc2_4bpp_srgb_block_img", VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG},
                  {"r16g16_s10_5_nv", VK_FORMAT_R16G16_S10_5_NV},
                  {"a1b5g5r5_unorm_pack16_khr", VK_FORMAT_A1B5G5R5_UNORM_PACK16_KHR},
                  {"a8_unorm_khr", VK_FORMAT_A8_UNORM_KHR},
            };

            if (const auto res = FormatDic.find(str); res != FormatDic.end()) {
                  return res->second;
            } else {
                  return VK_FORMAT_UNDEFINED;
            }
      }

      bool IsDepthOnlyFormat(VkFormat format) {
            return format == VK_FORMAT_D16_UNORM || format == VK_FORMAT_D32_SFLOAT;
      }

      bool IsStencilOnlyFormat(VkFormat format) {
            return format == VK_FORMAT_S8_UINT;
      }

      bool IsDepthStencilOnlyFormat(VkFormat format) {
            return format == VK_FORMAT_D16_UNORM_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT || format == VK_FORMAT_D32_SFLOAT_S8_UINT;
      }

      bool IsDepthStencilFormat(VkFormat format) {
            return IsDepthOnlyFormat(format) || IsDepthStencilOnlyFormat(format);
      }

      const char* GetImageLayoutString(VkImageLayout layout) {
            switch (layout) {
                  case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL: return "VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL";
                  case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: return "VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL";
                  case VK_IMAGE_LAYOUT_PREINITIALIZED: return "VK_IMAGE_LAYOUT_PREINITIALIZED";
                  case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL: return "VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL";
                  case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL: return "VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL";
                  case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL: return "VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL";
                  case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL: return "VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL";
                  case VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL: return "VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL";
                  case VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL: return "VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL";
                  case VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL: return "VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL";
                  case VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL: return "VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL";
                  case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR: return "VK_IMAGE_LAYOUT_PRESENT_SRC_KHR";
                  case VK_IMAGE_LAYOUT_VIDEO_DECODE_DST_KHR: return "VK_IMAGE_LAYOUT_VIDEO_DECODE_DST_KHR";
                  case VK_IMAGE_LAYOUT_VIDEO_DECODE_SRC_KHR: return "VK_IMAGE_LAYOUT_VIDEO_DECODE_SRC_KHR";
                  case VK_IMAGE_LAYOUT_VIDEO_DECODE_DPB_KHR: return "VK_IMAGE_LAYOUT_VIDEO_DECODE_DPB_KHR";
                  case VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR: return "VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR";
                  case VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT: return "VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT";
                  case VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR: return "VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR";
                  case VK_IMAGE_LAYOUT_RENDERING_LOCAL_READ_KHR: return "VK_IMAGE_LAYOUT_RENDERING_LOCAL_READ_KHR";
                  case VK_IMAGE_LAYOUT_VIDEO_ENCODE_DST_KHR: return "VK_IMAGE_LAYOUT_VIDEO_ENCODE_DST_KHR";
                  case VK_IMAGE_LAYOUT_VIDEO_ENCODE_SRC_KHR: return "VK_IMAGE_LAYOUT_VIDEO_ENCODE_SRC_KHR";
                  case VK_IMAGE_LAYOUT_VIDEO_ENCODE_DPB_KHR: return "VK_IMAGE_LAYOUT_VIDEO_ENCODE_DPB_KHR";
                  case VK_IMAGE_LAYOUT_ATTACHMENT_FEEDBACK_LOOP_OPTIMAL_EXT: return "VK_IMAGE_LAYOUT_ATTACHMENT_FEEDBACK_LOOP_OPTIMAL_EXT";
                  default: return "???";
            }
      }
}

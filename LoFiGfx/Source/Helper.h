//
// Created by starr on 2024/6/20.
//

#ifndef MARCOS_H
#define MARCOS_H

#define NO_COPY_MOVE_CONS(Class) \
Class(const Class&) = delete; \
Class(Class&&) = delete; \
Class& operator=(const Class&) = delete; \
Class& operator=(Class&&) = delete

#include <format>
#include <string>
#include <stdexcept>
#include <functional>
#include <vector>
#include <span>
#include <variant>
#include <ranges>
#include <optional>

#include "../Third/volk/volk.h"
#include "VmaLoader.h"
#include "entt/entt.hpp"
#include "Concurrent/concurrentqueue.h"

namespace LoFi {

      struct ContextSetupParam {
            bool Debug = false;
      };

      struct LayoutVariableBindInfo {
            std::string Name;
            entt::entity Buffer;
      };

      struct RenderPassBeginArgument {
            entt::entity TextureHandle = entt::null;
            bool ClearBeforeRendering = true;
            uint32_t ViewIndex = 0;
      };

      enum class ResourceUsage {
            NONE,
            TRANS_SRC,
            TRANS_DST,
            SAMPLED,
            READ_TEXTURE,
            WRITE_TEXTURE,
            READ_WRITE_TEXTURE,
            READ_BUFFER,
            WRITE_BUFFER,
            READ_WRITE_BUFFER,
            RENDER_TARGET,
            DEPTH_STENCIL,
            PRESENT,
            VERTEX_BUFFER,
            INDEX_BUFFER,
            INDIRECT_BUFFER,
      };

      enum KernelType {
            NONE,
            GRAPHICS,
            COMPUTE,
      };
}

namespace LoFi::Internal {
      VmaAllocator volkGetLoadedVmaAllocator();

      void volkLoadVmaAllocator(VmaAllocator allocator);

      entt::registry* volkGetLoadedEcsWorld();

      void volkLoadEcsWorld(entt::registry* world);

      VkPhysicalDevice volkGetLoadedPhysicalDevice();

      void volkLoadPhysicalDevice(VkPhysicalDevice device);

      const char* GetVkResultString(VkResult res);

      const char* GetVkFormatString(VkFormat format);

      const char* GetVkFormatStringSimpled(VkFormat format);

      VkFormat GetVkFormatFromString(const std::string& str);

      VkFormat GetVkFormatFromStringSimpled(const std::string& str);

      bool IsDepthOnlyFormat(VkFormat format);

      bool IsDepthStencilOnlyFormat(VkFormat format);

      bool IsDepthStencilFormat(VkFormat format);

      const char* GetImageLayoutString(VkImageLayout layout);

      const char* GetResourceUsageString(ResourceUsage stage);
}




#endif //MARCOS_H

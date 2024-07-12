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

//enable sse
#define GLM_FORCE_SWIZZLE
#define GLM_FORCE_INTRINSICS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

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

      enum class ContextResourceType {
            UNKONWN,
            WINDOW,
            IMAGE,
            BUFFER,
            IMAGE_VIEW,
            BUFFER_VIEW,
            PIPELINE,
            PIPELINE_LAYOUT
      };

      struct ContextResourceRecoveryInfo {
            ContextResourceType Type = ContextResourceType::UNKONWN;
            std::optional<size_t> Resource1{};
            std::optional<size_t> Resource2{};
            std::optional<size_t> Resource3{};
            std::optional<size_t> Resource4{};
      };

      class FreeList {
      public:
            uint32_t Gen() {
                  if (_free.empty()) {
                        return _top++;
                  } else {
                        uint32_t id = _free.back();
                        _free.pop_back();
                        return id;
                  }
            }

            void Free(uint32_t id) {
                  _free.push_back(id);
            }

            void Clear() {
                  _top = 0;
                  _free.clear();
            }

      private:
            uint32_t _top{};
            std::vector<uint32_t> _free{};
      };
}




#endif //MARCOS_H

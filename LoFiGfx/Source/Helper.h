//
// Created by starr on 2024/6/20.
//

#ifndef MARCOS_H
#define MARCOS_H

#ifndef GFX_SAFE_LEVEL
#define GFX_SAFE_LEVEL 0
#endif

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
#include "../Third/xxHash/xxh3.h"
#include "LoFiGfxDefines.h"

//enable sse
#define GLM_FORCE_SWIZZLE
#define GLM_FORCE_INTRINSICS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"


namespace LoFi {

      struct ResourceHandle {
            GfxEnumResourceType Type = GfxEnumResourceType::INVALID_RESOURCE_TYPE;
            entt::entity RHandle = entt::null;
      };

      struct ContextSetupParam {
            bool Debug = false;
      };
}

namespace LoFi::Internal {
      VmaAllocator volkGetLoadedVmaAllocator();

      void volkLoadVmaAllocator(VmaAllocator allocator);

      entt::registry* volkGetLoadedEcsWorld();

      void volkLoadEcsWorld(entt::registry* world);

      VkPhysicalDevice volkGetLoadedPhysicalDevice();

      void volkLoadPhysicalDevice(VkPhysicalDevice device);

      const char* ToStringVkResult(VkResult res);

      const char* ToStringVkFormat(VkFormat format);

      const char* ToStringVkFormatMini(VkFormat format);

      const char* ToStringResourceType(GfxEnumResourceType type);

      VkFormat FromStringVkFormat(const std::string& str);

      VkFormat FromStringVkFormatMini(const std::string& str);

      bool IsDepthOnlyFormat(VkFormat format);

      bool IsDepthStencilOnlyFormat(VkFormat format);

      bool IsDepthStencilFormat(VkFormat format);

      const char* GetImageLayoutString(VkImageLayout layout);

      const char* ToStringResourceUsage(GfxEnumResourceUsage stage);

      enum class ContextResourceType {
            UNKONWN,
            SWAPCHAIN,
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
            std::string ResourceName{};
      };

      class FreeList {
      public:
            uint32_t Gen() {
                  if(uint32_t value; !_free.try_dequeue(value)) {
                        _top += 1;
                        return _top;
                  } else {
                        return value;
                  }
            }

            void Free(uint32_t id) {
                  _free.enqueue(id);
            }

      private:
            moodycamel::ConcurrentQueue<uint32_t> _free{};
            std::atomic<uint32_t> _top {0};
      };

      struct HashResourceHandle {
            std::size_t operator()(const LoFi::ResourceHandle& s) const noexcept {
                  return XXH64(&s, sizeof(VkSamplerCreateInfo), 0);
            }
      };

      struct EqualResourceHandle {
            std::size_t operator()(const LoFi::ResourceHandle& a, const LoFi::ResourceHandle& b) const noexcept {
                  return a.Type == b.Type && a.RHandle == b.RHandle;
            }
      };
}




#endif //MARCOS_H

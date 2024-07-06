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
}

#endif //MARCOS_H

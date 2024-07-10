//
// Created by Arzuo on 2024/7/9.
//

#pragma once

#include "../Helper.h"

namespace LoFi::Component::Gfx {
      class Buffer;
      class FrameResource {
      public:
            NO_COPY_MOVE_CONS(FrameResource);

            ~FrameResource();

            explicit FrameResource(entt::entity id, uint64_t size, bool high_dynamic = true);

            [[nodiscard]] entt::entity GetHandle() const { return _id; }

            [[nodiscard]] entt::entity GetBuffer() const;

            [[nodiscard]] entt::entity GetBuffer(uint32_t idx) const { return _buffers.at(idx); }

            [[nodiscard]] uint32_t GetSize() const { return _dataCache.size(); }

            [[nodiscard]] VkDeviceAddress GetBufferAddress(uint32_t idx) const;

            void BarrierLayout(VkCommandBuffer cmd, KernelType new_kernel_type, ResourceUsage new_usage) const;

            void SetData(void* p, uint64_t size, uint64_t offset);

            static void UpdateAll();

      private:

            void UpdateFrameResource();

            entt::entity _id;

            std::vector<uint8_t> _dataCache{};

            std::array<entt::entity, 3> _buffers{};

            uint32_t _dirty = 0;

      };
}


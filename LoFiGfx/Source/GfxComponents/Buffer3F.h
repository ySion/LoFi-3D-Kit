//
// Created by Arzuo on 2024/7/9.
//

#pragma once

#include "../Helper.h"

namespace LoFi::Component::Gfx {
      class Buffer;
      class Buffer3F {
      public:
            NO_COPY_MOVE_CONS(Buffer3F);

            ~Buffer3F();

            explicit Buffer3F(entt::entity id);

            bool Init(const GfxParamCreateBuffer3F& param);

            [[nodiscard]] std::string& GetResourceName() { return _resourceName; }

            [[nodiscard]] ResourceHandle GetHandle() const { return {GfxEnumResourceType::Buffer3F, _id}; }

            [[nodiscard]] VkBuffer GetBuffer() const;

            [[nodiscard]] Buffer* GetBufferObject() const;
            
            [[nodiscard]] VkBuffer* GetBufferPtr() const;

            [[nodiscard]] uint32_t GetSize() const { return _dataCache.size(); }

            [[nodiscard]] VkDeviceAddress GetBufferAddress(uint32_t idx) const;

            [[nodiscard]] VkDeviceAddress GetCurrentBufferBDAAddress() const;

            void BarrierLayout(VkCommandBuffer cmd, GfxEnumKernelType new_kernel_type, GfxEnumResourceUsage new_usage) const;

            void SetData(const void* p, uint64_t size, uint64_t offset);

            bool Update(); // 如果依然有数据需要更新，返回true, 如果全更新完毕, 返回false

      private:

            entt::entity _id;

            std::vector<uint8_t> _dataCache{};

            std::unique_ptr<Buffer> _buffers[3];

            uint32_t _dirty = 0;

      private:
            std::string _resourceName{};

            bool _bNeedUpdate{};
      };
}


#pragma once

#include "Defines.h"
#include "../Helper.h"

namespace LoFi {
      class GfxContext;
}

namespace LoFi::Component::Gfx {
      class Buffer {
      public:
            static constexpr uint32_t MAX_UPDATE_BFFER_SIZE = 16 * 1024;

            NO_COPY_MOVE_CONS(Buffer);

            ~Buffer();

            explicit Buffer(const VkBufferCreateInfo& buffer_ci, const VmaAllocationCreateInfo& alloc_ci);

            explicit Buffer(entt::entity id, const VkBufferCreateInfo& buffer_ci, const VmaAllocationCreateInfo& alloc_ci);

            [[nodiscard]] VkBuffer GetBuffer() const { return _buffer; }

            [[nodiscard]] VkBuffer* GetBufferPtr() { return &_buffer; }

            [[nodiscard]] VkBufferView GetView() const { return _views.at(0); }

            [[nodiscard]] VkBufferView* GetViewPtr() { return &_views.at(0); }

            [[nodiscard]] VkBufferView GetView(uint32_t idx) const { return _views.at(idx); }

            [[nodiscard]] VkBufferView* GetViewPtr(uint32_t idx) { return &_views.at(idx); }

            [[nodiscard]] VkDeviceSize GetSize() const { return _vaildSize; }

            [[nodiscard]] VkDeviceSize GetCapacity() const { return _bufferCI->size; }

            [[nodiscard]] bool IsHostSide() const { return _isHostSide; }

            [[nodiscard]] VkDeviceAddress GetAddress() const;

            [[nodiscard]] entt::entity GetID() const { return _id; }

            VkBufferView CreateView(VkBufferViewCreateInfo view_ci);

            void ClearViews();

            void* Map();

            void Unmap();

            void SetData(const void* p, uint64_t size);

            void Recreate(uint64_t size);

            void BarrierLayout(VkCommandBuffer cmd, KernelType new_kernel_type, ResourceUsage new_usage);

      private:
            void CreateBuffer(const VkBufferCreateInfo& buffer_ci, const VmaAllocationCreateInfo& alloc_ci);

            void ReleaseAllViews() const;

            void RecreateAllViews();

            void CreateViewFromCurrentViewCIs();

            void Clean();

            void DestroyBuffer();

            friend class ::LoFi::GfxContext;

      private:
            entt::entity _id = entt::null;

            size_t _vaildSize{};

            bool _isHostSide = false;

            void* _mappedPtr{};

            VkBuffer _buffer{};

            VmaAllocation _memory{};

            VkDeviceAddress _address{};

            std::unique_ptr<VkBufferCreateInfo> _bufferCI{};

            std::unique_ptr<VmaAllocationCreateInfo> _memoryCI{};

            std::unique_ptr<Buffer> _intermediateBuffer{};

            std::vector<VkBufferView> _views{};

            std::vector<VkBufferViewCreateInfo> _viewCIs{};

            KernelType _currentKernelType = KernelType::NONE;

            ResourceUsage _currentUsage = ResourceUsage::NONE;

            std::vector<uint8_t> _dataCache{};
      };
}

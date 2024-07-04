#pragma once

#include "../Helper.h"

namespace LoFi {
      class Context;
}

namespace LoFi::Component {
      class Buffer {
      public:
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

            [[nodiscard]] std::optional<uint32_t> GetBindlessIndex() const { return _bindlessIndex; }

            [[nodiscard]] entt::entity GetID() const { return _id; }

            VkBufferView CreateView(VkBufferViewCreateInfo view_ci);

            void ClearViews();

            void* Map();

            void Unmap();

            void SetData(const void* p, uint64_t size);

            void Recreate(uint64_t size);

            void BarrierLayout(VkCommandBuffer cmd, VkPipelineStageFlags2 new_access, std::optional<VkAccessFlags2> src_layout,
                  std::optional<VkPipelineStageFlags2> src_stage, std::optional<VkPipelineStageFlags2> dst_stage);

      private:
            void CreateBuffer(const VkBufferCreateInfo& buffer_ci, const VmaAllocationCreateInfo& alloc_ci);

            void ReleaseAllViews() const;

            void RecreateAllViews();

            void CreateViewFromCurrentViewCIs();

            void Clean();

            void SetBindlessIndex(std::optional<uint32_t> bindless_index);

            void DestroyBuffer();

            friend class ::LoFi::Context;

      private:
            entt::entity _id = entt::null;

            size_t _vaildSize{};

            bool _isHostSide = false;

            void* _mappedPtr{};

            std::optional<uint32_t> _bindlessIndex{};

            VkBuffer _buffer{};

            VmaAllocation _memory{};

            std::unique_ptr<VkBufferCreateInfo> _bufferCI{};

            std::unique_ptr<VmaAllocationCreateInfo> _memoryCI{};

            std::unique_ptr<Buffer> _intermediateBuffer{};

            std::vector<VkBufferView> _views{};

            std::vector<VkBufferViewCreateInfo> _viewCIs{};

            VkAccessFlags2 _currentAccess {};
      };
}

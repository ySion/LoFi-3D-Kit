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

            [[nodiscard]] VkDeviceSize GetSize() const { return _bufferCI.size; }

            [[nodiscard]] bool IsHostSide() const { return _isHostSide; }

            [[nodiscard]] std::optional<uint32_t> GetBindlessIndex() const { return _bindlessIndex; }

            [[nodiscard]] entt::entity GetID() const { return _id; }

            VkBufferView CreateView(VkBufferViewCreateInfo view_ci);

            void ClearViews();

            void* Map();

            void Unmap();

            void SetData(void* p, uint64_t size);

            void Recreate(uint64_t size);

      private:

            void SetCallBackOnRecreate(const std::function<void(const Component::Buffer*)>& func);

            void CreateBuffer(const VkBufferCreateInfo& buffer_ci, const VmaAllocationCreateInfo& alloc_ci);

            void ReleaseAllViews() const;

            void RecreateAllViews();

            void CreateViewFromCurrentViewCIs();

            void Clean();

            void SetBindlessIndex(std::optional<uint32_t> bindless_index);

            friend class ::LoFi::Context;

      private:

            entt::entity _id = entt::null;

            bool _isHostSide = false;

            void* _mappedPtr{};

            std::optional<uint32_t> _bindlessIndex{};

            VkBuffer _buffer{};

            VmaAllocation _memory{};

            std::vector<VkBufferView> _views{};


            VkBufferCreateInfo _bufferCI{};

            VmaAllocationCreateInfo _memoryCI{};


            std::vector<VkBufferViewCreateInfo> _viewCIs{};

            std::unique_ptr<Buffer> _intermediateBuffer{};

            std::function<void(const Component::Buffer*)> _callBackOnRecreate;
      };
}
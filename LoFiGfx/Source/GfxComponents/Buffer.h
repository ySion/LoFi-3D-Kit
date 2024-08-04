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

            explicit Buffer();

            explicit Buffer(entt::entity id);

            bool Init(const GfxParamCreateBuffer& param);

            bool Init(const char* name, const VkBufferCreateInfo& ci, const VmaAllocationCreateInfo& mem_ci);

            std::string& GetResourceName() { return _resourceName; }

            [[nodiscard]] VkBuffer GetBuffer() const { return _buffer; }

            [[nodiscard]] VkBuffer* GetBufferPtr() { return &_buffer; }

            [[nodiscard]] VkBufferView GetView() const { return _views.at(0); }

            [[nodiscard]] VkBufferView* GetViewPtr() { return &_views.at(0); }

            [[nodiscard]] VkBufferView GetView(uint32_t idx) const { return _views.at(idx); }

            [[nodiscard]] VkBufferView* GetViewPtr(uint32_t idx) { return &_views.at(idx); }

            [[nodiscard]] VkDeviceSize GetCapacity() const { return _bufferCI->size; }

            [[nodiscard]] bool IsHostSide() const { return _isHostSide; }

            [[nodiscard]] VkDeviceAddress GetBDAAddress() const;

            [[nodiscard]] ResourceHandle GetHandle() const { return {GfxEnumResourceType::Buffer, _id}; }

            VkBufferView CreateView(VkBufferViewCreateInfo view_ci);

            void ClearViews();

            void* Map();

            void Unmap();

            bool SetData(const void* p, uint64_t size, uint8_t recursive_depth = 0);

            bool Recreate(uint64_t size);

            void BarrierLayout(VkCommandBuffer cmd, GfxEnumKernelType new_kernel_type, GfxEnumResourceUsage new_usage);

            void SetLayout(GfxEnumKernelType new_kernel_type, GfxEnumResourceUsage new_usage);

      private:
            bool CreateBuffer();

            void ReleaseAllViews() const;

            void RecreateAllViews();

            void CreateViewFromCurrentViewCIs();

            void Clean();

            void DestroyBuffer();

            void Update(VkCommandBuffer cmd);

            friend class ::LoFi::GfxContext;

      private:
            entt::entity _id = entt::null;

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

            GfxEnumKernelType _currentKernelType = GfxEnumKernelType::OUT_OF_KERNEL;

            GfxEnumResourceUsage _currentUsage = GfxEnumResourceUsage::UNKNOWN_RESOURCE_USAGE;

            std::vector<uint8_t> _dataCache{};

            std::function<void(VkCommandBuffer)> _updateCommand{};

      private:
            std::string _resourceName{};

            bool _bNeedUpdate {};
      };
}

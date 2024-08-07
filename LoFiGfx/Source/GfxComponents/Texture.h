#pragma once

#include "../Helper.h"
#include "Buffer.h"
#include "Defines.h"


namespace LoFi {
      class GfxContext;
}

namespace LoFi::Component::Gfx {
      class Texture {
      public:
            NO_COPY_MOVE_CONS(Texture);

            ~Texture();

            explicit Texture();

            explicit Texture(entt::entity id);

            bool Init(const VkImageCreateInfo& image_ci, VkImage Image, bool isBorrow, const char* name = nullptr);

            bool Init(const VkImageCreateInfo& image_ci, const VmaAllocationCreateInfo& alloc_ci, const GfxParamCreateTexture2D& param);

            std::string& GetResourceName() { return _resourceName; }

            [[nodiscard]] VkImage GetImage() const { return _image; }

            [[nodiscard]] VkImage* GetImagePtr() { return &_image; }

            [[nodiscard]] VkImageView GetView() const { return _views.at(0); }

            [[nodiscard]] VkImageView GetView(uint32_t idx) const { return _views.at(idx); }

            [[nodiscard]] VkImageView* GetViewPtr() { return &_views.at(0); }

            [[nodiscard]] VkImageView* GetViewPtr(uint32_t idx) { return &_views.at(idx); }

            [[nodiscard]] bool IsBorrowed() const { return _isBorrow; }

            [[nodiscard]] ResourceHandle GetHandle() const { return {GfxEnumResourceType::Texture2D, _id}; }

            [[nodiscard]] VkExtent3D GetExtent() const { return _imageCI->extent; }

            [[nodiscard]] VkFormat GetFormat() const { return _imageCI->format; }

            [[nodiscard]] bool IsTextureFormatColor() const { return !Internal::IsDepthStencilFormat(_imageCI->format); }

            [[nodiscard]] bool IsTextureFormatDepthOnly() const { return Internal::IsDepthOnlyFormat(_imageCI->format); }

            [[nodiscard]] bool IsTextureFormatDepthStencilOnly() const { return Internal::IsDepthStencilOnlyFormat(_imageCI->format); }

            [[nodiscard]] bool IsTextureFormatDepthStencil() const { return Internal::IsDepthStencilFormat(_imageCI->format); }

            [[nodiscard]] GfxEnumResourceUsage GetCurrentUsage() const { return _currentUsage; }

            [[nodiscard]] VkSampler GetSampler() const { return _sampler; }

            [[nodiscard]] std::optional<uint32_t> GetBindlessIndex() const { return _bindlessIndex; }

            [[nodiscard]] const VkImageViewCreateInfo& GetViewCI() const { return _viewCIs.at(0); }

            [[nodiscard]] const VkImageViewCreateInfo& GetViewCI(uint32_t idx) const { return _viewCIs.at(idx); }

            VkImageView CreateView(VkImageViewCreateInfo view_ci);

            void ClearViews();

            bool Resize(uint32_t w, uint32_t h);

            void SetData(const void* data, size_t size);

            void BarrierLayout(VkCommandBuffer cmd, GfxEnumKernelType new_kernel_type, GfxEnumResourceUsage new_usage);

            void SetLayout(GfxEnumKernelType new_kernel_type, GfxEnumResourceUsage new_usage);

      private:

            void SetSampler(VkSampler sampler) { _sampler = sampler; }

            void ReleaseAllViews() const;

            void Clean();

            void SetBindlessIndex(std::optional<uint32_t> index);

            void DestroyTexture();

            void Update(VkCommandBuffer cmd);

            friend class Swapchain;

            friend class ::LoFi::GfxContext;

      private:
            entt::entity _id = entt::null;

            bool _isBorrow{};

            std::optional<uint32_t> _bindlessIndex{};

            VkImage _image{};

            VmaAllocation _memory{};

            std::vector<VkImageView> _views{};

            std::unique_ptr<VkImageCreateInfo> _imageCI{};

            std::unique_ptr<VmaAllocationCreateInfo> _memoryCI{};

            std::vector<VkImageViewCreateInfo> _viewCIs{};

            VkSampler _sampler{};

            GfxEnumKernelType _currentKernelType = GfxEnumKernelType::OUT_OF_KERNEL;

            GfxEnumResourceUsage _currentUsage = GfxEnumResourceUsage::UNKNOWN_RESOURCE_USAGE;

            std::unique_ptr<Buffer> _intermediateBuffer{};

            std::vector<uint8_t> _dataCache{};

            std::function<void(VkCommandBuffer)> _updateCommand{};

      private:
            std::string _resourceName{};

            bool _bNeedUpdate{};
      };
}

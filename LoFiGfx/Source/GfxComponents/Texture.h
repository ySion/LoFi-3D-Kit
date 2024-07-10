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

            explicit Texture(const VkImageCreateInfo& image_ci, VkImage Image, bool isBorrow);

            explicit Texture(entt::entity id, const VkImageCreateInfo& image_ci, const VmaAllocationCreateInfo& alloc_ci);

            [[nodiscard]] VkImage GetImage() const { return _image; }

            [[nodiscard]] VkImage* GetImagePtr() { return &_image; }

            [[nodiscard]] VkImageView GetView() const { return _views.at(0); }

            [[nodiscard]] VkImageView GetView(uint32_t idx) const { return _views.at(idx); }

            [[nodiscard]] VkImageView* GetViewPtr() { return &_views.at(0); }

            [[nodiscard]] VkImageView* GetViewPtr(uint32_t idx) { return &_views.at(idx); }

            [[nodiscard]] bool IsBorrowed() const { return _isBorrow; }

            [[nodiscard]] entt::entity GetID() const { return _id; }

            [[nodiscard]] VkExtent3D GetExtent() const { return _imageCI->extent; }

            [[nodiscard]] VkFormat GetFormat() const { return _imageCI->format; }

            [[nodiscard]] bool IsTextureFormatColor() const { return !Internal::IsDepthStencilFormat(_imageCI->format); }

            [[nodiscard]] bool IsTextureFormatDepthOnly() const { return !Internal::IsDepthStencilOnlyFormat(_imageCI->format); }

            [[nodiscard]] bool IsTextureFormatDepthStencil() const { return !Internal::IsDepthStencilFormat(_imageCI->format); }

            [[nodiscard]] ResourceUsage GetCurrentUsage() const { return _currentUsage; }

            [[nodiscard]] VkSampler GetSampler() const { return _sampler; }

            [[nodiscard]] std::optional<uint32_t> GetBindlessIndexForSampler() const { return _bindlessIndexForSampler; }

            [[nodiscard]] std::optional<uint32_t> GetBindlessIndexForStorage() const { return _bindlessIndexForStorage; }

            [[nodiscard]] const VkImageViewCreateInfo& GetViewCI() const { return _viewCIs.at(0); }

            [[nodiscard]] const VkImageViewCreateInfo& GetViewCI(uint32_t idx) const { return _viewCIs.at(idx); }

            VkImageView CreateView(VkImageViewCreateInfo view_ci);

            void ClearViews();

            void SetData(const void* data, size_t size);

            void BarrierLayout(VkCommandBuffer cmd, KernelType new_kernel_type, ResourceUsage new_usage);

      private:
            void SetSampler(VkSampler sampler) { _sampler = sampler; }

            void ReleaseAllViews() const;

            void Clean();

            void SetBindlessIndexForSampler(std::optional<uint32_t> index);

            void SetBindlessIndexForStorage(std::optional<uint32_t> index);

            void DestroyTexture();

            friend class Swapchain;

            friend class ::LoFi::GfxContext;

      private:
            entt::entity _id;

            bool _isBorrow{};

            std::optional<uint32_t> _bindlessIndexForSampler{};

            std::optional<uint32_t> _bindlessIndexForStorage{};

            VkImage _image{};

            VmaAllocation _memory{};

            std::vector<VkImageView> _views{};

            std::unique_ptr<VkImageCreateInfo> _imageCI{};

            std::vector<VkImageViewCreateInfo> _viewCIs{};

            VkSampler _sampler{};

            KernelType _currentKernelType = KernelType::NONE;

            ResourceUsage _currentUsage = ResourceUsage::NONE;

            std::unique_ptr<Buffer> _intermediateBuffer{};
      };
}

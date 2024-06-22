#pragma once

#include "../Helper.h"

namespace LoFi {
      class Context;
}

namespace LoFi::Component {

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

            [[nodiscard]] VkExtent3D GetExtent() const { return _imageCI.extent; }

            [[nodiscard]] VkFormat GetFormat() const { return _imageCI.format; }

            [[nodiscard]] VkImageLayout GetLayout() const { return _currentLayout; }

            [[nodiscard]] VkSampler GetSampler() const { return _sampler; }

            [[nodiscard]] std::optional<uint32_t> GetBindlessIndexForSampler() const { return _bindlessIndexForSampler; }

            [[nodiscard]] std::optional<uint32_t> GetBindlessIndexForComputeKernel() const { return _bindlessIndexForComputeKernel; }

            VkImageView CreateView(VkImageViewCreateInfo view_ci);

            void ClearViews();

            // [[nodiscard]] VkImageMemoryBarrier2KHR Barrier(VkImageLayout newLayout);

      private:

            [[nodiscard]] VkPipelineStageFlags2& CurrentAccessMask() { return _currentAccessMask; }

            [[nodiscard]] VkImageLayout& CurrentLayout() { return _currentLayout; }

            void SetSampler(VkSampler sampler) { _sampler = sampler; }

            void ReleaseAllViews() const;

            void Clean();

            void SetBindlessIndexForSampler(std::optional<uint32_t> index);

            void SetBindlessIndexForComputeKernel(std::optional<uint32_t> index);

            friend class Swapchain;

            friend class ::LoFi::Context;

      private:
            entt::entity _id;

            bool _isBorrow{};

            std::optional<uint32_t> _bindlessIndexForSampler{};

            std::optional<uint32_t> _bindlessIndexForComputeKernel{};

            VkImage _image{};

            VmaAllocation _memory{};

            std::vector<VkImageView> _views{};


            VkImageCreateInfo _imageCI{};

            std::vector<VkImageViewCreateInfo> _viewCIs{};

            VkSampler _sampler{};


            VkAccessFlags2 _currentAccessMask;

            VkImageLayout _currentLayout;
      };
}
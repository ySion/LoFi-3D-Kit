#pragma once

#include "../Helper.h"
#include "Buffer.h"


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

            [[nodiscard]] VkExtent3D GetExtent() const { return _imageCI->extent; }

            [[nodiscard]] VkFormat GetFormat() const { return _imageCI->format; }

            [[nodiscard]] VkImageLayout GetCurrentLayout() const { return _currentLayout; }

            [[nodiscard]] VkSampler GetSampler() const { return _sampler; }

            [[nodiscard]] std::optional<uint32_t> GetBindlessIndexForSampler() const { return _bindlessIndexForSampler; }

            [[nodiscard]] std::optional<uint32_t> GetBindlessIndexForComputeKernel() const { return _bindlessIndexForComputeKernel; }

            [[nodiscard]] const VkImageViewCreateInfo& GetViewCI() const {return _viewCIs.at(0); }

            [[nodiscard]] const VkImageViewCreateInfo& GetViewCI(uint32_t idx) const {return _viewCIs.at(idx); }

            VkImageView CreateView(VkImageViewCreateInfo view_ci);

            void ClearViews();

            void SetData(void* data , size_t size);

            void BarrierLayout(VkCommandBuffer cmd, VkImageLayout new_layout, std::optional<VkImageLayout> src_layout = std::nullopt,
                               std::optional<VkPipelineStageFlags2> src_stage = std::nullopt,
                               std::optional<VkPipelineStageFlags2> dst_stage = std::nullopt);


            // [[nodiscard]] VkImageMemoryBarrier2KHR Barrier(VkImageLayout newLayout);

      private:

            void SetSampler(VkSampler sampler) { _sampler = sampler; }

            void ReleaseAllViews() const;

            void Clean();

            void SetBindlessIndexForSampler(std::optional<uint32_t> index);

            void SetBindlessIndexForComputeKernel(std::optional<uint32_t> index);

            void DestroyTexture();

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

            std::unique_ptr<VkImageCreateInfo> _imageCI{};

            std::vector<VkImageViewCreateInfo> _viewCIs{};

            VkSampler _sampler{};

            VkImageLayout _currentLayout;

            std::unique_ptr<Buffer> _intermediateBuffer{};

      };
}
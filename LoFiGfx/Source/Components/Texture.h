#pragma once

#include "../Helper.h"

namespace LoFi::Component {

      class Texture {
      public:
            NO_COPY_MOVE_CONS(Texture);

            ~Texture();

            explicit Texture(const VkImageCreateInfo& image_ci, VkImage Image, bool isBorrow);

            explicit Texture(const VkImageCreateInfo& image_ci, const VmaAllocationCreateInfo& alloc_ci);

            [[nodiscard]] VkImage GetImage() const { return _image; }

            [[nodiscard]] VkImage* GetImagePtr() { return &_image; }

            [[nodiscard]] VkImageView GetView() const { return _views.at(0); }

            [[nodiscard]] VkImageView GetView(int idx) const { return _views.at(idx); }

            [[nodiscard]] VkImageView* GetViewPtr() { return &_views.at(0); }

            [[nodiscard]] VkImageView* GetViewPtr(int idx) { return &_views.at(idx); }

            [[nodiscard]] bool IsBorrowed() const { return _isBorrow; }

            [[nodiscard]] VkExtent3D GetExtent() const { return _imageCI.extent; }

            [[nodiscard]] VkFormat GetFormat() const { return _imageCI.format; }

            VkImageView CreateView(VkImageViewCreateInfo view_ci);

            void ClearViews();

           // [[nodiscard]] VkImageMemoryBarrier2KHR Barrier(VkImageLayout newLayout);

      private:

            [[nodiscard]] VkPipelineStageFlags2& CurrentAccessMask() { return _currentAccessMask; }

            [[nodiscard]] VkImageLayout& CurrentLayout() { return _currentLayout; }

            void ReleaseAllViews() const;

            void Clean();

            friend class Swapchain;

      private:

            bool _isBorrow{};

            VkImage _image{};

            VmaAllocation _memory{};

            std::vector<VkImageView> _views{};


            VkImageCreateInfo _imageCI{};

            std::vector<VkImageViewCreateInfo> _viewCIs{};


            VkAccessFlags2 _currentAccessMask;

            VkImageLayout _currentLayout;
      };
}
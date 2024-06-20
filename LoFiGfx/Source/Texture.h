//
// Created by starr on 2024/6/20.
//

#ifndef TEXTURE_H
#define TEXTURE_H

#include <vector>

#include "Marcos.h"

namespace LoFi {
      class Texture {
      public:
            NO_COPY_MOVE_CONS(Texture);

            ~Texture();

            bool IsBorrowed() const;

            Texture(const VkImageCreateInfo& image_ci, VkImage Image, bool isBorrow);

            Texture(const VkImageCreateInfo& image_ci, const VmaAllocationCreateInfo& alloc_ci);

            operator VkImageView() const;

            operator VkImageView*();

            operator VkImage() const;

            operator VkImage*();

            VkImage GetImage() const;

            VkImageView GetView() const;

            VkImageView GetView(int idx) const;

            VkImageView CreateView(VkImageViewCreateInfo view_ci);

            void ClearViewExceptMainView();

            void ClearAllViews();

            void Resize(int w, int h, int depth = 0);

            void Recreate(const VkImageCreateInfo& image_ci, VkImage Image, bool isBorrow);

            void Recreate(const VkImageCreateInfo& image_ci, const VmaAllocationCreateInfo& alloc_ci);

            VkExtent3D GetExtent() const;

            VkFormat GetFormat() const;

      private:

            void CreateImage(const VkImageCreateInfo& image_ci, VkImage Image, bool isBorrow);

            void CreateImage(const VkImageCreateInfo& image_ci, const VmaAllocationCreateInfo& alloc_ci);

            void RecreateAllViews();

            void ReleaseAllViews();

            void CreateViewFromCurrentViewCIs();

            void CleanUp() const;

      private:

            bool _isBorrow {};
            VkImage _image {};
            VmaAllocation _memory{};
            std::vector<VkImageView> _views{};

            VkImageCreateInfo _imageCI{};
            VmaAllocationCreateInfo _memoryCI{};
            std::vector<VkImageViewCreateInfo> _viewCIs{};
      };
}




#endif //TEXTURE_H

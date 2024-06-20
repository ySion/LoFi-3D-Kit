//
// Created by starr on 2024/6/20.
//

#include "Texture.h"
#include "Message.h"

#include <stdexcept>
using namespace LoFi;

Texture::~Texture() {
     CleanUp();
}

Texture::Texture(const VkImageCreateInfo &image_ci, VkImage Image, bool isBorrow) {
      CreateImage(image_ci, Image, isBorrow);
}

Texture::Texture(const VkImageCreateInfo &image_ci, const VmaAllocationCreateInfo &alloc_ci) {
      CreateImage(image_ci, alloc_ci);
}

Texture::operator VkImageView() const {
      return _views.at(0);
}

Texture::operator VkImageView *() {
      return &_views.at(0);
}

Texture::operator VkImage_T *() const {
      return _image;
}

Texture::operator VkImage *() {
      return &_image;
}

VkImageView Texture::CreateView(VkImageViewCreateInfo view_ci) {
      view_ci.image = _image;

      VkImageView view{};
      if (vkCreateImageView(volkGetLoadedDevice(), &view_ci, nullptr, &view) != VK_SUCCESS) {
            const std::string msg = "Failed to create image view";
            MessageManager::addMessage(MessageType::Error, msg);
            throw std::runtime_error(msg);
      }

      _views.push_back(view);
      _viewCIs.push_back(view_ci);
      return view;
}

void Texture::ClearViewExceptMainView() {
      for (int i = 1; i < _views.size(); i++) {
            vkDestroyImageView(volkGetLoadedDevice(), _views.at(i), nullptr);
      }
}

void Texture::ClearAllViews() {
      for (const auto view: _views) {
            vkDestroyImageView(volkGetLoadedDevice(), view, nullptr);
      }
}

void Texture::Recreate(const VkImageCreateInfo &image_ci, VkImage Image, bool isBorrow) {
      CleanUp();
      CreateImage(image_ci, Image, isBorrow);
}

void Texture::Recreate(const VkImageCreateInfo &image_ci, const VmaAllocationCreateInfo &alloc_ci) {
      CleanUp();
      CreateImage(image_ci, alloc_ci);
}

void Texture::Resize(int w, int h, int depth) {
      if(_isBorrow) {
            std::string msg = "Cannot resize borrowed image";
            MessageManager::addMessage(MessageType::Error, msg);
            throw std::runtime_error(msg);
      }

      ReleaseAllViews();

      const auto allocator = volkGetLoadedVmaAllocator();
      _imageCI.extent = VkExtent3D{(uint32_t)w, (uint32_t)h, depth == 0 ? 1 : _imageCI.extent.depth};
      if (vmaCreateImage(allocator, &_imageCI, &_memoryCI, &_image, &_memory, nullptr) != VK_SUCCESS) {
            const std::string msg = "Failed to create image";
            MessageManager::addMessage(MessageType::Error, msg);
            throw std::runtime_error(msg);
      }

      CreateViewFromCurrentViewCIs();
}

VkExtent3D Texture::GetExtent() const {
      return _imageCI.extent;
}

VkFormat Texture::GetFormat() const {
      return _imageCI.format;
}

void Texture::CreateImage(const VkImageCreateInfo &image_ci, VkImage Image, bool isBorrow) {
      if (!Image) {
            const std::string msg = "Invalid image";
            MessageManager::addMessage(MessageType::Error, msg);
            throw std::runtime_error(msg);
      }
      _isBorrow = isBorrow;
      _image = Image;
      _imageCI = image_ci;
}

void Texture::CreateImage(const VkImageCreateInfo &image_ci, const VmaAllocationCreateInfo &alloc_ci) {
      const auto allocator = volkGetLoadedVmaAllocator();
      if (vmaCreateImage(allocator, &image_ci, &alloc_ci, &_image, &_memory, nullptr) != VK_SUCCESS) {
            const std::string msg = "Failed to create image";
            MessageManager::addMessage(MessageType::Error, msg);
            throw std::runtime_error(msg);
      }
}

void Texture::RecreateAllViews() {
      ReleaseAllViews();
      _views.clear();
      CreateViewFromCurrentViewCIs();
}

void Texture::ReleaseAllViews() {
      for (const auto view: _views) {
            vkDestroyImageView(volkGetLoadedDevice(), view, nullptr);
      }
}

void Texture::CreateViewFromCurrentViewCIs() {
      for (auto &view_ci: _viewCIs) {
            VkImageView view{};
            view_ci.image = _image;
            if (vkCreateImageView(volkGetLoadedDevice(), &view_ci, nullptr, &view) != VK_SUCCESS) {
                  const std::string msg = "Failed to create image view";
                  MessageManager::addMessage(MessageType::Error, msg);
                  throw std::runtime_error(msg);
            }
      }
}

void Texture::CleanUp() const {
      const auto device = volkGetLoadedDevice();

      for (const auto view: _views) {
            vkDestroyImageView(device, view, nullptr);
      }

      if (!_isBorrow) {
            vkDestroyImage(device, _image, nullptr);
      }
}


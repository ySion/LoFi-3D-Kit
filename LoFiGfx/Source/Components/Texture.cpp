#include "Texture.h"
#include "../Message.h"
#include "../Context.h"

LoFi::Component::Texture::~Texture() {
      Clean();
}

LoFi::Component::Texture::Texture(const VkImageCreateInfo& image_ci, VkImage Image, bool isBorrow) {

      _isBorrow = isBorrow;

      _image = Image;
      _imageCI = image_ci;

      _currentAccessMask = 0;
      _currentLayout = _imageCI.initialLayout;
}

LoFi::Component::Texture::Texture(entt::entity id, const VkImageCreateInfo& image_ci, const VmaAllocationCreateInfo& alloc_ci) {
      const auto allocator = volkGetLoadedVmaAllocator();

      if (vmaCreateImage(allocator, &image_ci, &alloc_ci, &_image, &_memory, nullptr) != VK_SUCCESS) {
            const std::string msg = "Failed to create image";
            MessageManager::Log(MessageType::Error, msg);
            throw std::runtime_error(msg);
      }
      _id = id;
      _currentAccessMask = 0;
      _currentLayout = _imageCI.initialLayout;
}

VkImageView LoFi::Component::Texture::CreateView(VkImageViewCreateInfo view_ci) {

      view_ci.image = _image;

      VkImageView view{};
      if (vkCreateImageView(volkGetLoadedDevice(), &view_ci, nullptr, &view) != VK_SUCCESS) {
            const std::string msg = "Failed to create image view";
            MessageManager::Log(MessageType::Error, msg);
            throw std::runtime_error(msg);
      }
      _views.push_back(view);
      _viewCIs.push_back(view_ci);

      return view;
}

void LoFi::Component::Texture::ClearViews() {
      ReleaseAllViews();
      _views.clear();
      _viewCIs.clear();
}

void LoFi::Component::Texture::ReleaseAllViews() const {
      for (const auto view : _views) {
            ContextResourceRecoveryInfo info {
                  .Type = ContextResourceType::IMAGEVIEW,
                  .Resource1 = (size_t)view
            };
            Context::Get()->RecoveryContextResource(info);
      }
}

void LoFi::Component::Texture::Clean() {
      ClearViews();

      _currentAccessMask = 0;
      _currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;

      if (!_isBorrow) {
            DestroyTexture();
      }
}

void LoFi::Component::Texture::SetBindlessIndexForSampler(std::optional<uint32_t> index)
{
      _bindlessIndexForSampler = index;
}

void LoFi::Component::Texture::SetBindlessIndexForComputeKernel(std::optional<uint32_t> index)
{
      _bindlessIndexForComputeKernel = index;
}

void LoFi::Component::Texture::DestroyTexture() {
      ContextResourceRecoveryInfo info {
            .Type = ContextResourceType::IMAGE,
            .Resource1 = (size_t)_image,
            .Resource2 = (size_t)_memory,
            .Resource3 = _bindlessIndexForComputeKernel,
            .Resource4 = _bindlessIndexForSampler
      };
      Context::Get()->RecoveryContextResource(info);
}

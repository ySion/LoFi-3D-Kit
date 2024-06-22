#include "Swapchain.h"
#include "Window.h"
#include "../Message.h"
#include "SDL3/SDL_vulkan.h"


using namespace LoFi::Component;
//
//Swapchain::Swapchain(Swapchain&& other) noexcept
//{
//      _id = other._id;
//      _surface = other._surface;
//      _swapchain = other._swapchain;
//      _currentFrameIndex = other._currentFrameIndex;
//      _currentImageIndex = other._currentImageIndex;
//      for (int i = 0; i < 3; i++) {
//            _imageAvailableSemaphores[i] = other._imageAvailableSemaphores[i];
//      }
//      _images = std::move(other._images);
//
//      _id = entt::null;
//}
//
//Swapchain& Swapchain::operator=(Swapchain&& other) noexcept {
//      _id = other._id;
//      _surface = other._surface;
//      _swapchain = other._swapchain;
//      _currentFrameIndex = other._currentFrameIndex;
//      _currentImageIndex = other._currentImageIndex;
//      for (int i = 0; i < 3; i++) {
//            _imageAvailableSemaphores[i] = other._imageAvailableSemaphores[i];
//      }
//      _images = std::move(other._images);
//
//      _id = entt::null;
//      return *this;
//}

Swapchain::Swapchain(entt::entity id) : _id(id) {

      auto device = volkGetLoadedDevice();

      VkSemaphoreCreateInfo semaphore_ci{};
      semaphore_ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

      for (auto& _imageAvailableSemaphore : _imageAvailableSemaphores) {
            VkSemaphore semaphore{};
            if (auto res = vkCreateSemaphore(device, &semaphore_ci, nullptr, &semaphore); res != VK_SUCCESS) {
                  std::string msg = std::format("Swapchain::Swapchain - Failed to create semaphore {}", (int64_t)res);
                  MessageManager::Log(MessageType::Error, msg);
                  throw std::runtime_error(msg);
            }
            _imageAvailableSemaphore = semaphore;
      }

      CreateOrRecreateSwapChain();
}

VkImageMemoryBarrier2 Swapchain::GenerateCurrentRenderTargetBarrier() const
{
      auto& current_access_mask = GetCurrentRenderTarget()->CurrentAccessMask();
      auto& current_layout = GetCurrentRenderTarget()->CurrentLayout();
      auto image = GetCurrentRenderTarget()->GetImage();

      VkImageMemoryBarrier2 barrier = {};
      barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR;
      barrier.pNext = nullptr;
      barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      barrier.subresourceRange.baseMipLevel = 0;
      barrier.subresourceRange.levelCount = 1;
      barrier.subresourceRange.baseArrayLayer = 0;
      barrier.subresourceRange.layerCount = 1;
      barrier.srcQueueFamilyIndex = 0;
      barrier.dstQueueFamilyIndex = 0;
      barrier.image = image;

      if (current_layout == VK_IMAGE_LAYOUT_UNDEFINED || current_layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
            barrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
            barrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;

            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;

            barrier.oldLayout = current_layout;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

            current_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            current_access_mask = VK_ACCESS_2_TRANSFER_WRITE_BIT;

      } else if (current_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            barrier.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;

            barrier.srcAccessMask = current_access_mask;
            barrier.dstAccessMask = 0;

            barrier.oldLayout = current_layout;
            barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

            current_layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            current_access_mask = 0;
      } else {
            std::string msg = "Swapchain::GenerateCurrentRenderTargetBarrier - Invalid layout";
            MessageManager::Log(MessageType::Error, msg);
            throw std::runtime_error(msg);
      }

      return barrier;
}

void Swapchain::AcquireNextImage() {
      _currentFrameIndex = (_currentFrameIndex + 1) % 3;

      auto res = vkAcquireNextImageKHR(volkGetLoadedDevice(), _swapchain, UINT64_MAX, _imageAvailableSemaphores[_currentFrameIndex], nullptr, &_currentImageIndex);

      if (res != VK_SUCCESS) {
            if (res == VK_ERROR_OUT_OF_DATE_KHR) {
                  CreateOrRecreateSwapChain();
                  res = vkAcquireNextImageKHR(volkGetLoadedDevice(), _swapchain, UINT64_MAX, _imageAvailableSemaphores[_currentFrameIndex], nullptr, &_currentImageIndex);
                  if (res != VK_SUCCESS) {
                        std::string msg = "Swapchain::AcquireNextImage - Failed to acquire next image";
                        MessageManager::Log(MessageType::Error, msg);
                        throw std::runtime_error(msg);
                  }
            }
      }
}

Swapchain::~Swapchain() {

      const auto device = volkGetLoadedDevice();
      const auto instance = volkGetLoadedInstance();

      for (auto semaphore : _imageAvailableSemaphores) {
            vkDestroySemaphore(device, semaphore, nullptr);
      }

      _images.clear();

      vkDestroySwapchainKHR(device, _swapchain, nullptr);
      vkDestroySurfaceKHR(instance, _surface, nullptr);
}

void Swapchain::CreateOrRecreateSwapChain() {
      entt::registry& world = *volkGetLoadedEcsWorld();

      if (!world.valid(_id)) {
            auto str = std::format("Swapchain::CreateOrRecreateSwapChain - Invalid Entity {}", (uint32_t)_id);
            MessageManager::Log(MessageType::Error, str);
            throw std::runtime_error(str);
      }

      auto res = world.try_get<Window>(_id);

      if (!res) {
            auto str = std::format("Swapchain::CreateOrRecreateSwapChain - Entity {} Missing Window", (uint32_t)_id);
            MessageManager::Log(MessageType::Error, str);
            throw std::runtime_error(str);
      }

      auto window_size = res->GetSize();
      auto win_ptr = res->GetWindowPtr();

      if (!SDL_Vulkan_CreateSurface(win_ptr, volkGetLoadedInstance(), nullptr, &_surface)) {
            std::string msg = "Swapchain::CreateOrRecreateSwapChain - Failed to create surface";
            MessageManager::Log(MessageType::Error, msg);
            throw std::runtime_error(msg);
      }

      VkSwapchainCreateInfoKHR sp_ci{};
      sp_ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
      sp_ci.surface = _surface;
      sp_ci.minImageCount = 3;
      sp_ci.imageFormat = VK_FORMAT_B8G8R8A8_UNORM;
      sp_ci.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
      sp_ci.imageExtent = window_size;
      sp_ci.imageArrayLayers = 1;
      sp_ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
      sp_ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
      sp_ci.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
      sp_ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
      sp_ci.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
      sp_ci.clipped = VK_TRUE;

      auto device = volkGetLoadedDevice();

      if (auto res = vkCreateSwapchainKHR(device, &sp_ci, nullptr, &_swapchain); res != VK_SUCCESS) {
            std::string msg = std::format("Swapchain::CreateOrRecreateSwapChain - Failed to create swapchain {}", (int64_t)res);
            MessageManager::Log(MessageType::Error, msg);
            throw std::runtime_error(msg);
      }

      uint32_t image_count = 0;
      vkGetSwapchainImagesKHR(device, _swapchain, &image_count, nullptr);
      std::vector<VkImage> images(image_count);
      vkGetSwapchainImagesKHR(device, _swapchain, &image_count, images.data());

      VkImageCreateInfo image_ci{};
      image_ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
      image_ci.imageType = VK_IMAGE_TYPE_2D;
      image_ci.format = VK_FORMAT_B8G8R8A8_UNORM;
      image_ci.extent.width = window_size.width;
      image_ci.extent.height = window_size.height;
      image_ci.extent.depth = 1;
      image_ci.mipLevels = 1;
      image_ci.arrayLayers = 1;
      image_ci.samples = VK_SAMPLE_COUNT_1_BIT;
      image_ci.tiling = VK_IMAGE_TILING_OPTIMAL;
      image_ci.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
      image_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

      VkImageViewCreateInfo view_ci{};
      view_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      view_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
      view_ci.format = VK_FORMAT_B8G8R8A8_UNORM;
      view_ci.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
      view_ci.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
      view_ci.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
      view_ci.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
      view_ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      view_ci.subresourceRange.baseMipLevel = 0;
      view_ci.subresourceRange.levelCount = 1;
      view_ci.subresourceRange.baseArrayLayer = 0;
      view_ci.subresourceRange.layerCount = 1;
      view_ci.image = nullptr; //auto fill

      for (const auto image : images) {
            auto texture = std::make_unique<Texture>(image_ci, image, true);
            texture->CreateView(view_ci);
            _images.push_back(std::move(texture));
      }
}

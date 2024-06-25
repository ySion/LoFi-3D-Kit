#include "Swapchain.h"
#include "Window.h"
#include "../Message.h"
#include "SDL3/SDL_vulkan.h"


using namespace LoFi::Component;

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

void Swapchain::BarrierCurrentRenderTarget(VkCommandBuffer cmd) const {
      auto render_traget = GetCurrentRenderTarget();
      auto current_layout = render_traget->GetCurrentLayout();

      if(current_layout == VK_IMAGE_LAYOUT_UNDEFINED || current_layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
            render_traget->BarrierLayout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, std::nullopt, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
      } else if(current_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            render_traget->BarrierLayout(cmd, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, std::nullopt, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
      } else {
            //err
            auto str = std::format("Swapchain::BarrierCurrentRenderTarget - Invalid layout {}", GetImageLayoutString(current_layout));
            MessageManager::Log(MessageType::Error, str);
      }

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

      auto current_window_size = res->GetSize();

      if(current_window_size.width == 0 || current_window_size.height == 0) {
            return;
      }

      if(current_window_size.width == _extent.width && current_window_size.height == _extent.height) {
            return;
      }


      auto device = volkGetLoadedDevice();
      auto instance = volkGetLoadedInstance();

      vkDeviceWaitIdle(device);

      _images.clear();

      vkDestroySwapchainKHR(device, _swapchain, nullptr);
      vkDestroySurfaceKHR(instance, _surface, nullptr);

      auto win_ptr = res->GetWindowPtr();

      if (!SDL_Vulkan_CreateSurface(win_ptr, instance, nullptr, &_surface)) {
            std::string msg = "Swapchain::CreateOrRecreateSwapChain - Failed to create surface";
            MessageManager::Log(MessageType::Error, msg);
            throw std::runtime_error(msg);
      }

      VkSurfaceCapabilitiesKHR surface_capabilities{};

      vkGetPhysicalDeviceSurfaceCapabilitiesKHR(volkGetLoadedPhysicalDevice(), _surface, &surface_capabilities);

      VkSwapchainCreateInfoKHR sp_ci{};
      sp_ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
      sp_ci.surface = _surface;
      sp_ci.minImageCount = 3;
      sp_ci.imageFormat = VK_FORMAT_B8G8R8A8_UNORM;
      sp_ci.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
      sp_ci.imageExtent = surface_capabilities.currentExtent;
      sp_ci.imageArrayLayers = 1;
      sp_ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
      sp_ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
      sp_ci.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
      sp_ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
      sp_ci.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
      sp_ci.clipped = VK_TRUE;



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
      image_ci.extent.width = surface_capabilities.currentExtent.width;
      image_ci.extent.height = surface_capabilities.currentExtent.height;
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

      _extent = surface_capabilities.currentExtent;
}

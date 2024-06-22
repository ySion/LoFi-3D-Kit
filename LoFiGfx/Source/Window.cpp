//
// Created by starr on 2024/6/20.
//

#include "Window.h"
#include "Message.h"
#include "Context.h"

#include <stdexcept>
#include <format>

#include "SDL3/SDL.h"
#include "SDL3/SDL_vulkan.h"

using namespace LoFi;

Window::Window(const char *title, int width, int height) {
      _w = width;
      _h = height;
      _window = SDL_CreateWindow(title, width, height, SDL_WINDOW_VULKAN);
      SDL_SetWindowMinimumSize(_window, 128, 128);
      SDL_SetWindowResizable(_window, true);
      CreateOrRecreateSwapChain();
}

void Window::Resize(int w, int h) const {
      if(_w == w && _h == h) return;
      SDL_SetWindowSize(_window, w, h);
}

void Window::SetFullScreen(bool full_screen) {
      SDL_SetWindowFullscreen(_window, full_screen);
}

void Window::Update() {
      int x,y;
      SDL_GetWindowSize(_window, &x, &y);
      if(x != _w || y != _h) {
            _w = x;
            _h = y;
            CreateOrRecreateSwapChain();
      }
}

int32_t Window::GetWindowID() const {
      return (uint32_t)SDL_GetWindowID(_window);
}

VkSemaphore Window::GetCurrentSemaphore() const {
      return _imageAvailableSemaphores[_currentFrameIndex];
}

Texture * Window::GetCurrentRenderTarget() const {
      return _images[_currentImageIndex].get();
}

uint32_t Window::GetCurrentRenderTargetIndex() const {
      return _currentImageIndex;
}

VkSwapchainKHR Window::GetSwapChain() const {
      return _swapchain;
}

VkImageMemoryBarrier2 Window::GenerateBarrier() const
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

      if(current_layout == VK_IMAGE_LAYOUT_UNDEFINED || current_layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
            barrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
            barrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;

            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;

            barrier.oldLayout = current_layout;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

            current_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            current_access_mask = VK_ACCESS_2_TRANSFER_WRITE_BIT;

      } else if(current_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            barrier.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;

            barrier.srcAccessMask = current_access_mask;
            barrier.dstAccessMask = 0;

            barrier.oldLayout = current_layout;
            barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

            current_layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            current_access_mask = 0;
      } else {
            std::string msg = "Invalid layout";
            MessageManager::addMessage(MessageType::Error, msg);
            throw std::runtime_error(msg);
      }

      return barrier;
}


void Window::AcquireNextImage() {
      _currentFrameIndex ++;
      _currentFrameIndex %= _images.size();
      auto res = vkAcquireNextImageKHR(volkGetLoadedDevice(), _swapchain, UINT64_MAX, _imageAvailableSemaphores.at(_currentFrameIndex), nullptr, &_currentImageIndex);

      if(res != VK_SUCCESS) {
            if (res == VK_ERROR_OUT_OF_DATE_KHR) {
                  CreateOrRecreateSwapChain();
                  res = vkAcquireNextImageKHR(volkGetLoadedDevice(), _swapchain, UINT64_MAX, _imageAvailableSemaphores.at(_currentFrameIndex), nullptr, &_currentImageIndex);
                  if (res != VK_SUCCESS) {
                        std::string msg = "Failed to acquire next image";
                        MessageManager::addMessage(MessageType::Error, msg);
                        throw std::runtime_error(msg);
                  }
            }
      }
}

Window::~Window() {
      const auto device = volkGetLoadedDevice();
      const auto instance = volkGetLoadedInstance();

      for (auto semaphore: _imageAvailableSemaphores) {
            vkDestroySemaphore(device, semaphore, nullptr);
      }

      _images.clear();
      vkDestroySwapchainKHR(device, _swapchain, nullptr);
      vkDestroySurfaceKHR(instance, _surface, nullptr);
      SDL_DestroyWindow(_window);
}

void Window::CreateOrRecreateSwapChain() {
      const auto device = volkGetLoadedDevice();
      const auto instance = volkGetLoadedInstance();

      _images.clear();

      if (_swapchain) {
            vkDestroySwapchainKHR(device, _swapchain, nullptr);
            vkDestroySurfaceKHR(instance, _surface, nullptr);
            _swapchain = nullptr;
      }

      if (!SDL_Vulkan_CreateSurface(_window, instance, nullptr, &_surface)) {
            std::string msg = "Failed to create surface";
            MessageManager::addMessage(MessageType::Error, msg);
            throw std::runtime_error(msg);
      }

      VkSwapchainCreateInfoKHR sp_ci{};
      sp_ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
      sp_ci.surface = _surface;
      sp_ci.minImageCount = 3;
      sp_ci.imageFormat = VK_FORMAT_B8G8R8A8_UNORM;
      sp_ci.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
      sp_ci.imageExtent.width = _w;
      sp_ci.imageExtent.height = _h;
      sp_ci.imageArrayLayers = 1;
      sp_ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
      sp_ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
      sp_ci.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
      sp_ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
      sp_ci.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
      sp_ci.clipped = VK_TRUE;

      if (auto res = vkCreateSwapchainKHR(device, &sp_ci, nullptr, &_swapchain); res != VK_SUCCESS) {
            std::string msg = std::format("Failed to create swapchain {}", (int64_t)res);
            MessageManager::addMessage(MessageType::Error, msg);
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
      image_ci.extent.width = _w;
      image_ci.extent.height = _h;
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

      VkSemaphoreCreateInfo semaphore_ci{};
      semaphore_ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

      for (const auto image : images) {
            auto texture = std::make_unique<Texture>(image_ci, image, true);
            texture->CreateView(view_ci);
            _images.push_back(std::move(texture));
      }

      if (_imageAvailableSemaphores.size() == 0) {
            for (const auto& image : images) {
                  if (auto res = vkCreateSemaphore(device, &semaphore_ci, nullptr, &_imageAvailableSemaphores.emplace_back()); res != VK_SUCCESS) {
                        std::string msg = std::format("Failed to create semaphore {}", (int64_t)res);
                        MessageManager::addMessage(MessageType::Error, msg);
                        throw std::runtime_error(msg);
                  }
            }
      }

      MessageManager::addMessage(MessageType::Normal, std::format("Resize Window to {}x{}", _w, _h));
}

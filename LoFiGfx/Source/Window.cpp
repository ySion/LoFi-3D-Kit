//
// Created by starr on 2024/6/20.
//

#include "Window.h"

#include <format>

#include "Message.h"

#include <stdexcept>


#include "SDL3/SDL.h"
#include "SDL3/SDL_vulkan.h"

using namespace LoFi;

Window::Window(const char *title, int width, int height) {
      _window = SDL_CreateWindow(title, width, height, SDL_WINDOW_VULKAN);
      SDL_SetWindowMinimumSize(_window, 128, 128);
      SDL_SetWindowResizable(_window, true);
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
      return SDL_GetWindowID(_window);
}

Window::~Window() {
      const auto device = volkGetLoadedDevice();
      const auto instance = volkGetLoadedInstance();
      _images.clear();
      vkDestroySwapchainKHR(device, _swapchain, nullptr);
      vkDestroySurfaceKHR(instance, _surface, nullptr);
      SDL_DestroyWindow(_window);
}

void Window::CreateOrRecreateSwapChain() {
      const auto device = volkGetLoadedDevice();
      const auto instance = volkGetLoadedInstance();

      _images.clear();

      if(_swapchain) {
            vkDestroySwapchainKHR(device, _swapchain, nullptr);
            vkDestroySurfaceKHR(instance, _surface, nullptr);
            _swapchain = nullptr;
      }

      if (!SDL_Vulkan_CreateSurface(_window, instance, nullptr, &_surface)) {
            std::string msg = "Failed to create surface";
            MessageManager::addMessage(MessageType::Error, msg);
            throw std::runtime_error(msg);
      }

      VkSwapchainCreateInfoKHR sp_ci {};
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

      if(auto res = vkCreateSwapchainKHR(device, &sp_ci, nullptr, &_swapchain); res != VK_SUCCESS) {
            std::string msg = std::format("Failed to create swapchain {}", (int64_t)res) ;
            MessageManager::addMessage(MessageType::Error, msg);
            throw std::runtime_error(msg);
      }

      uint32_t image_count = 0;
      vkGetSwapchainImagesKHR(device, _swapchain, &image_count, nullptr);
      std::vector<VkImage> images(image_count);
      vkGetSwapchainImagesKHR(device, _swapchain, &image_count, images.data());

      VkImageCreateInfo image_ci {};
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

      VkImageViewCreateInfo view_ci {};
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

      images.clear();
      for (const auto image: images) {
            auto texture = std::make_unique<Texture>(image_ci, image, true);
            texture->CreateView(view_ci);
            _images.push_back(std::move(texture));
      }

      MessageManager::addMessage(MessageType::Normal, std::format("Resize Window to {}x{}", _w, _h));
}

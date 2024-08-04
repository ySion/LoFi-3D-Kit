#include "Swapchain.h"
#include "../GfxContext.h"
#include "../Message.h"
#include "LoFiGfxDefines.h"

using namespace LoFi::Component::Gfx;
using namespace LoFi::Internal;

Swapchain::Swapchain(entt::entity id, const GfxParamCreateSwapchain& param) : _id(id) {
      _resourceName = param.pResourceName ? param.pResourceName : std::string{};

      _onResizeCallBack = param.PtrOnSwapchainNeedResizeCallback;
      _anyHandleForResizeCallback = param.AnyHandleForResizeCallback;

      auto device = volkGetLoadedDevice();

      VkSemaphoreCreateInfo semaphore_ci{};
      semaphore_ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

      for (auto& _imageAvailableSemaphore : _imageAvailableSemaphores) {
            VkSemaphore semaphore{};
            if (auto res = vkCreateSemaphore(device, &semaphore_ci, nullptr, &semaphore); res != VK_SUCCESS) {
                  std::string err = std::format("[SwapchainCreate] vkCreateSemaphore Failed, return {}.", ToStringVkResult(res));
                  if(!_resourceName.empty()) err += std::format(" - Name: \"{}\"", _resourceName);
                  MessageManager::Log(MessageType::Error, err);
                  throw std::runtime_error(err);
            }
            _imageAvailableSemaphore = semaphore;
      }

      CreateOrRecreateSwapChain();
      AcquireNextImage();
}

void Swapchain::AcquireNextImage() {
      _currentFrameIndex = (_currentFrameIndex + 1) % 3;

      if (_preAccquireResult == VK_ERROR_OUT_OF_DATE_KHR || _preAccquireResult == VK_SUBOPTIMAL_KHR) {
            CreateOrRecreateSwapChain();
      }

      auto res = vkAcquireNextImageKHR(volkGetLoadedDevice(), _swapchain, UINT64_MAX, _imageAvailableSemaphores[_currentFrameIndex], nullptr, &_currentImageIndex);

      if (res != VK_SUCCESS) {
            if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR) {
                  // will recreate swapchain in next frame
            } else {
                  auto err = std::format("[Swapchain::AcquireNextImage] Failed to acquire next image {}.", ToStringVkResult(res));
                  if(!_resourceName.empty())err += std::format(" - Name: \"{}\"", _resourceName);
                  MessageManager::Log(MessageType::Error, err);
            }
      }
      _preAccquireResult = res;
}

void Swapchain::PresentBarrier(VkCommandBuffer cmd) const {
      GetCurrentRenderTarget()->BarrierLayout(cmd, GfxEnumKernelType::OUT_OF_KERNEL, GfxEnumResourceUsage::PRESENT);
}

Swapchain::~Swapchain() {
      const auto device = volkGetLoadedDevice();
      const auto instance = volkGetLoadedInstance();

      vkDeviceWaitIdle(device);

      for (auto semaphore : _imageAvailableSemaphores) {
            vkDestroySemaphore(device, semaphore, nullptr);
      }

      _images.clear();

      vkDestroySwapchainKHR(device, _swapchain, nullptr);
      vkDestroySurfaceKHR(instance, _surface, nullptr);
}

bool Swapchain::CreateOrRecreateSwapChain() {
      if(_onResizeCallBack == nullptr) {
            std::string err = "[Swapchain::CreateOrRecreateSwapChain] delegate \"OnResizeCallBack\" is null, can't resize swapchain.";
            if(!_resourceName.empty())err += std::format(" - Name: \"{}\"", _resourceName);
            MessageManager::Log(MessageType::Warning, err);
            return false;
      }

      auto device = volkGetLoadedDevice();
      auto instance = volkGetLoadedInstance();

      vkDeviceWaitIdle(device);

      auto new_surface = (VkSurfaceKHR)_onResizeCallBack(_anyHandleForResizeCallback, (uint64_t)volkGetLoadedInstance());
      VkSurfaceCapabilitiesKHR surface_capabilities{};
      vkGetPhysicalDeviceSurfaceCapabilitiesKHR(volkGetLoadedPhysicalDevice(), new_surface, &surface_capabilities);

      if(surface_capabilities.currentExtent.width <= 0 || surface_capabilities.currentExtent.height <= 0) {
            _preAccquireResult = VK_SUCCESS;
            return true;
      }


      if(!_images.empty()) {
            for (auto& image : _images) {
                  GfxContext::Get()->RemoveBindlessIndexTextureImmediately(image.get());
            }
      }
      _images.clear();

      vkDestroySwapchainKHR(device, _swapchain, nullptr);
      vkDestroySurfaceKHR(instance, _surface, nullptr);

      //auto win_ptr = window_comp->GetWindowPtr();

      _surface = new_surface;

      if(_surface == nullptr) {
            std::string err = "[Swapchain::CreateOrRecreateSwapChain] Invaild Surface Handle.";
            if(!_resourceName.empty())err += std::format(" - Name: \"{}\"", _resourceName);
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      VkSwapchainCreateInfoKHR sp_ci{};
      sp_ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
      sp_ci.surface = _surface;
      sp_ci.minImageCount = 3;
      sp_ci.imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
      sp_ci.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
      sp_ci.imageExtent = surface_capabilities.currentExtent;
      sp_ci.imageArrayLayers = 1;
      sp_ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
      sp_ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
      sp_ci.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
      sp_ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
      sp_ci.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
      sp_ci.clipped = VK_TRUE;

      if (const auto res = vkCreateSwapchainKHR(device, &sp_ci, nullptr, &_swapchain); res != VK_SUCCESS) {
            std::string err = std::format("[Swapchain::CreateOrRecreateSwapChain] vkCreateSwapchainKHR Failed, return {}.", ToStringVkResult(res));
            if(!_resourceName.empty())err += std::format(" - Name: \"{}\"", _resourceName);
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      uint32_t image_count = 0;
      vkGetSwapchainImagesKHR(device, _swapchain, &image_count, nullptr);
      std::vector<VkImage> images(image_count);
      vkGetSwapchainImagesKHR(device, _swapchain, &image_count, images.data());

      VkImageCreateInfo image_ci{};
      image_ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
      image_ci.imageType = VK_IMAGE_TYPE_2D;
      image_ci.format = VK_FORMAT_R8G8B8A8_UNORM;
      image_ci.extent.width = surface_capabilities.currentExtent.width;
      image_ci.extent.height = surface_capabilities.currentExtent.height;
      image_ci.extent.depth = 1;
      image_ci.mipLevels = 1;
      image_ci.arrayLayers = 1;
      image_ci.samples = VK_SAMPLE_COUNT_1_BIT;
      image_ci.tiling = VK_IMAGE_TILING_OPTIMAL;
      image_ci.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
      image_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

      VkImageViewCreateInfo view_ci{};
      view_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      view_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
      view_ci.format = VK_FORMAT_R8G8B8A8_UNORM;
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

      uint32_t name_idx = 0;
      for (const auto image : images) {
            const std::string name = std::format("{}_backbuffer_{}", _resourceName, name_idx++);
            auto texture = std::make_unique<Texture>();
            texture->Init(image_ci, image, true, name.c_str());
            texture->CreateView(view_ci);
            GfxContext::Get()->MakeBindlessIndexTexture(texture.get());
            _images.push_back(std::move(texture));
      }

      _extent = surface_capabilities.currentExtent;

      _preAccquireResult = VK_SUCCESS;
      return true;
}

#pragma once

#include "../Helper.h"
#include "Texture.h"

namespace LoFi::Component::Gfx {
      class Swapchain {
      public:
            NO_COPY_MOVE_CONS(Swapchain);

            explicit Swapchain(entt::entity id, const GfxParamCreateSwapchain& param);

            ~Swapchain();

            [[nodiscard]] VkSurfaceKHR GetSurface() const { return _surface; }

            [[nodiscard]] VkSurfaceKHR* GetSurfacePtr() { return &_surface; }

            [[nodiscard]] VkSwapchainKHR GetSwapchain() const { return _swapchain; }

            [[nodiscard]] VkSwapchainKHR* GetSwapchainPtr() { return &_swapchain; }

            [[nodiscard]] VkSemaphore GetCurrentSemaphore() const { return _imageAvailableSemaphores[_currentFrameIndex]; }

            [[nodiscard]] uint32_t GetCurrentSemaphoreIndex() const { return _currentFrameIndex; }

            [[nodiscard]] Texture* GetCurrentRenderTarget() const { return _images.at(_currentImageIndex).get(); }

            [[nodiscard]] uint32_t GetCurrentRenderTargetIndex() const { return _currentImageIndex; }

            void AcquireNextImage();

            void PresentBarrier(VkCommandBuffer cmd) const;

      private:

            void CreateOrRecreateSwapChain();

            //friend class Window;

            //friend class ::LoFi::Context;

      private:
            entt::entity _id{};

            VkSurfaceKHR _surface{};

            VkSwapchainKHR _swapchain{};

            VkSemaphore _imageAvailableSemaphores[3]{};

            uint8_t _currentFrameIndex{};

            uint32_t _currentImageIndex{};

            std::vector<std::unique_ptr<Texture>> _images{};

            VkExtent2D _extent{};

            VkResult _preAccquireResult = VK_SUCCESS;

            uint64_t _anyHandleForResizeCallback {};

            uint64_t(*_onResizeCallBack)(uint64_t, uint64_t) = nullptr;

            std::string _resourceName{};
      };
}

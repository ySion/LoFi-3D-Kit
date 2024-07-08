#pragma once

#include "../Helper.h"
#include "Texture.h"

namespace LoFi::Component {
      class Swapchain {
      public:
            NO_COPY_MOVE_CONS(Swapchain);

            Swapchain(entt::entity id);

            ~Swapchain();

            [[nodiscard]] VkSurfaceKHR GetSurface() const { return _surface; }

            [[nodiscard]] VkSurfaceKHR* GetSurfacePtr() { return &_surface; }

            [[nodiscard]] VkSwapchainKHR GetSwapchain() const { return _swapchain; }

            [[nodiscard]] VkSwapchainKHR* GetSwapchainPtr() { return &_swapchain; }

            [[nodiscard]] VkSemaphore GetCurrentSemaphore() const { return _imageAvailableSemaphores[_currentFrameIndex]; }

            [[nodiscard]] Texture* GetCurrentRenderTarget() const { return _images.at(_currentImageIndex).get(); }

            [[nodiscard]] uint32_t GetCurrentRenderTargetIndex() const { return _currentImageIndex; }

            void AcquireNextImage();

            void SetMappedRenderTarget(entt::entity texture);

            void BeginFrame(VkCommandBuffer cmd) const;

            void EndFrame(VkCommandBuffer cmd) const;

      private:
            void MapRenderTarget(VkCommandBuffer cmd) const;

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

            entt::entity _mappedRenderTarget = entt::null;
      };
}

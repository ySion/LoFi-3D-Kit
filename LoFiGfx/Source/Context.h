//
// Created by starr on 2024/6/20.
//
#define VK_NO_PROTOTYPES

#include <unordered_map>
#include <unordered_set>

#include "Helper.h"
#include "Window.h"

namespace LoFi {


      class Context {
      public:
            Context() = default;

            ~Context();

            NO_COPY_MOVE_CONS(Context)

            void EnableDebug();

            void Init();

            void Shutdown();

            uint32_t CreateWindow(const char* title, int w, int h);

            void DestroyWindow(uint32_t ptr);

            Texture* CreateRenderTexture(int w, int h);

            Texture* CreateDepthStencil(int w, int h);

            void DestroyTexture(Texture* texture);

            void* PollEvent();
      private:
            bool _bDebugMode = false;

            VkInstance _instance;
            VkPhysicalDevice _physicalDevice;
            VkDevice _device;
            VmaAllocator _allocator;
            VkQueue _queue;
            VkCommandPool _commandPool;
            VkCommandBuffer _commandBuffer[3];
            VkFence _mainCommandFence[3];
            VkSemaphore _mainCommandSemaphore[3];

            std::unordered_map<Texture*, std::unique_ptr<Texture>> _textures;
            std::unordered_map<uint32_t, std::unique_ptr<Window>> _windows{};
      };
}

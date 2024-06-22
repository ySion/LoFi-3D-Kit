//
// Created by starr on 2024/6/20.
//
#pragma once

#include "Helper.h"

#include "Components/Window.h"
#include "Components/Swapchain.h"
#include "Components/Texture.h"
#include "Components/Buffer.h"

struct IDxcLibrary;
struct IDxcCompiler3;
struct IDxcUtils;

namespace LoFi {

      struct ContextSetupParam {
            bool Debug = false;
      };

      class Context {
            static Context* GlobalContext;

      public:
            static Context* Get() { return GlobalContext;}

            NO_COPY_MOVE_CONS(Context);

            Context();

            ~Context();

            void Init();

            void Shutdown();

            entt::entity CreateWindow(const char* title, int w, int h);

            void DestroyWindow(uint32_t id);

            void DestroyWindow(entt::entity window);

            [[nodiscard]] entt::entity CreateRenderTexture(int w, int h);

            [[nodiscard]] entt::entity CreateDepthStencil(int w, int h);

            [[nodiscard]] entt::entity CreateVertexBuffer(uint64_t size, bool high_dynamic = false);

            [[nodiscard]] entt::entity CreateVertexBuffer(void* data, uint64_t size, bool high_dynamic = false);



            //void DestroyTexture(Texture* texture);

            void* PollEvent();

            void EnqueueCommand(const std::function<void(VkCommandBuffer)>& command);

            void BeginFrame();

            void EndFrame();

           // Program* CreateProgram(const char* source_code, const char* type, const char* entry = "main");

      private:

            void PrepareWindowRenderTarget();

            [[nodiscard]] uint32_t GetCurrentFrameIndex() const;

            [[nodiscard]] VkCommandBuffer GetCurrentCommandBuffer() const;

            [[nodiscard]] VkFence GetCurrentFence() const;

            void GoNextFrame();

      private:

            bool _bDebugMode = true;

            VkInstance _instance{};

            VkPhysicalDevice _physicalDevice{};

            VkDevice _device{};

            VmaAllocator _allocator{};

            VkQueue _queue{};

            VkCommandPool _commandPool{};

            VkCommandBuffer _commandBuffer[3]{};

            VkFence _mainCommandFence[3]{};

            VkSemaphore _mainCommandQueueSemaphore[3]{};

            uint32_t _currentCommandBufferIndex = 0;

            uint64_t _sumFrameCount = 0;

      private:
            std::vector<std::function<void(VkCommandBuffer)>> _commandQueue;

           /* std::unordered_map<Texture*, std::unique_ptr<Texture>> _textures;
            std::unordered_map<Buffer*, std::unique_ptr<Buffer>> _buffers;
            std::unordered_map<uint32_t, std::unique_ptr<Window>> _windows{};*/

            entt::dense_map<uint32_t, entt::entity> _windowIdToWindow{};

            IDxcLibrary* _dxcLibrary {};
            IDxcCompiler3* _dxcCompiler {};
            IDxcUtils* _dxcUtils{};

            entt::registry _world;
      };
}

//
// Created by starr on 2024/6/20.
//
#pragma once

#include "Helper.h"
#include "PhysicalDevice.h"

#include "Components/Window.h"
#include "Components/Swapchain.h"
#include "Components/Texture.h"
#include "Components/Buffer.h"
#include "Components/Program.h"
#include "../Third/xxHash/xxh3.h"

struct IDxcLibrary;
struct IDxcCompiler3;
struct IDxcUtils;

namespace LoFi {

      struct ContextSetupParam {
            bool Debug = false;
      };


      class FreeList {
      public:
            uint32_t Gen() {
                  if (_free.empty()) {
                        return _top++;
                  } else {
                        uint32_t id = _free.back();
                        _free.pop_back();
                        return id;
                  }
            }

            void Free(uint32_t id) {
                  _free.push_back(id);
            }

            void Clear() {
                  _top = 0;
                  _free.clear();
            }

      private:
            uint32_t _top {};
            std::vector<uint32_t> _free {};
      };

      class Context {

            friend class Component::Buffer;
            friend class Component::Program;

            struct SamplerCIHash {
                  std::size_t operator()(const VkSamplerCreateInfo& s) const noexcept {
                        return XXH64(&s, sizeof(VkSamplerCreateInfo), 0);
                  }
            };

            struct SamplerCIEqual {
                  std::size_t operator()(const VkSamplerCreateInfo& a, const VkSamplerCreateInfo& b) const noexcept {
                        return memcmp(&a, &b, sizeof(VkSamplerCreateInfo));
                  }
            };

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


           /* [[nodiscard]] entt::entity  CreateTexture2DArray();

            [[nodiscard]] entt::entity  CreateTexture3D();

            [[nodiscard]] entt::entity  CreateTextureCube();*/

            [[nodiscard]] entt::entity CreateTexture2D(VkFormat format, int w, int h, int mipMapCounts = 1);

            [[nodiscard]] entt::entity CreateBuffer(uint64_t size, bool cpu_access = false);

            [[nodiscard]] entt::entity CreateBuffer(void* data, uint64_t size, bool cpu_access = false);

            [[nodiscard]] entt::entity CreateGraphicKernel(entt::entity program);

            [[nodiscard]] entt::entity CreateProgram(std::string_view source_code);

            void DestroyBuffer(entt::entity buffer);

            void DestroyTexture(entt::entity texture);

            void* PollEvent();

            void EnqueueCommand(const std::function<void(VkCommandBuffer)>& command);

            void BeginFrame();

            void EndFrame();



           // Program* CreateProgram(const char* source_code, const char* type, const char* entry = "main");

      private:

            void MakeBindlessIndexBuffer(entt::entity buffer, std::optional<uint32_t> specifyindex = {});

            void MakeBindlessIndexTextureForSampler(entt::entity texture, uint32_t viewIndex = 0, std::optional<uint32_t> specifyindex = {});

            void MakeBindlessIndexTextureForComputeKernel(entt::entity texture, uint32_t viewIndex = 0, std::optional<uint32_t> specifyindex = {});

            void SetTextureSampler(entt::entity image, const VkSamplerCreateInfo& sampler_ci);

            void PrepareWindowRenderTarget();

            [[nodiscard]] uint32_t GetCurrentFrameIndex() const;

            [[nodiscard]] VkCommandBuffer GetCurrentCommandBuffer() const;

            [[nodiscard]] VkFence GetCurrentFence() const;

            void GoNextFrame();

      private:

            bool _bDebugMode = true;

            VkInstance _instance{};

            VkPhysicalDevice _physicalDevice{};

            PhysicalDevice _physicalDeviceAbility{};

            VkDevice _device{};

            VmaAllocator _allocator{};

            VkQueue _queue{};

            VkCommandPool _commandPool{};

            VkCommandBuffer _commandBuffer[3]{};

            VkFence _mainCommandFence[3]{};

            VkSemaphore _mainCommandQueueSemaphore[3]{};

            uint32_t _currentCommandBufferIndex = 0;

            uint64_t _sumFrameCount = 0;

            //Descriptors

            VkDescriptorPool _descriptorPool{};

            VkDescriptorSetLayout _bindlessDescriptorSetLayout{};

            VkDescriptorSet _bindlessDescriptorSet{};

            //Normal Sampler
            VkSampler _defaultSampler{};

            entt::dense_map<VkSamplerCreateInfo, VkSampler, SamplerCIHash, SamplerCIEqual> _samplers{};

            FreeList _bindlessIndexFreeList[3]{};

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

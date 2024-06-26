//
// Created by starr on 2024/6/20.
//
#pragma once

#include <span>
#include <variant>

#include "Helper.h"
#include "PhysicalDevice.h"

#include "Components/Window.h"
#include "Components/Swapchain.h"
#include "Components/Texture.h"
#include "Components/Buffer.h"
#include "Components/Program.h"
#include "Components/GraphicKernel.h"
#include "../Third/xxHash/xxh3.h"

struct IDxcLibrary;
struct IDxcCompiler3;
struct IDxcUtils;

namespace LoFi {

      struct ContextSetupParam {
            bool Debug = false;
      };

      enum class ContextResourceType {
            UNKONWN,
            WINDOW,
            IMAGE,
            BUFFER,
            IMAGEVIEW,
            BUFFERVIEW
      };

      struct ContextResourceRecoveryInfo {
            ContextResourceType Type = ContextResourceType::UNKONWN;
            std::optional<size_t> Resource1 {};
            std::optional<size_t> Resource2 {};
            std::optional<size_t> Resource3 {};
            std::optional<size_t> Resource4 {};
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
            friend class Component::GraphicKernel;
            friend class Component::Texture;

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

            entt::entity CreateWindow(const char* title, int w, int h);

           /* [[nodiscard]] entt::entity  CreateTexture2DArray();

            [[nodiscard]] entt::entity  CreateTexture3D();

            [[nodiscard]] entt::entity  CreateTextureCube();*/

            [[nodiscard]] entt::entity CreateTexture2D(VkFormat format, int w, int h, int mipMapCounts = 1);

            [[nodiscard]] entt::entity CreateBuffer(uint64_t size, bool cpu_access = false, bool bindless = true);

            [[nodiscard]] entt::entity CreateBuffer(void* data, uint64_t size, bool cpu_access = false, bool bindless = true);

            [[nodiscard]] entt::entity CreateGraphicKernel(entt::entity program);

            [[nodiscard]] entt::entity CreateProgram(std::string_view source_code);

            void DestroyBuffer(entt::entity buffer);

            void DestroyTexture(entt::entity texture);

            void SetBufferData(entt::entity buffer, void* data, uint64_t size);

            void SetTexture2DData(entt::entity texture, void* data, uint64_t size);

            void SetTexture2DData(entt::entity texture, entt::entity buffer);

            void EnqueueCommand(const std::function<void(VkCommandBuffer)>& command);

            void* PollEvent();

            void BeginFrame();

            void EndFrame();

            void BeginRenderPass();

            void EndRenderPass();

            void DestroyWindow(uint32_t id);

            void DestroyWindow(entt::entity window);

            void BindRenderTargetBeforeRenderPass(entt::entity color_texture, bool clear = true, uint32_t view_index = 0);

            void BindDepthStencilTargetBeforeRenderPass(entt::entity depth_stencil_texture, bool clear = true, uint32_t view_index = 0);

            void BindStencilTargetToRenderPass(entt::entity stencil_texture, bool clear = true, uint32_t view_index = 0);

            void BindDepthTargetToRenderPass(entt::entity depth_texture, bool clear = true, uint32_t view_index = 0);

            void BindGraphicKernelToRenderPass(entt::entity kernel);

            //
            // void CmdBindVertexBuffer(entt::entity buffer);
            //
            // void CmdBindIndexBuffer(entt::entity buffer);
            //
            // void CmdBindRenderTarget(entt::entity texture);
            //
            // void CmdBindDepthStencil(entt::entity texture);
            //
            // void CmdBindTexture(entt::entity texture, uint32_t position = 0);
            //
            // void CmdBindTextures(std::span<entt::entity> textures);
            //
            // void CmdSetPushConstant(void* data, uint32_t size);
            //
            // void CmdBindBuffer(entt::entity buffer, uint32_t position = 0);

      private:
            void Shutdown();

            void RecoveryContextResource(const ContextResourceRecoveryInfo& pack);

            uint32_t MakeBindlessIndexBuffer(entt::entity buffer);

            uint32_t MakeBindlessIndexTextureForSampler(entt::entity texture, uint32_t viewIndex = 0);

            uint32_t MakeBindlessIndexTextureForComputeKernel(entt::entity texture, uint32_t viewIndex = 0);

            void SetTextureSampler(entt::entity image, const VkSamplerCreateInfo& sampler_ci);

            void PrepareWindowRenderTarget();

            uint32_t GetCurrentFrameIndex() const;

            VkCommandBuffer GetCurrentCommandBuffer() const;

            VkFence GetCurrentFence() const;

            void GoNextFrame();

            void StageRecoveryContextResource();

            void RecoveryAllContextResourceImmediately();

            void RecoveryContextResourceWindow(const ContextResourceRecoveryInfo& pack);

            void RecoveryContextResourceBuffer(const ContextResourceRecoveryInfo& pack);

            void RecoveryContextResourceBufferView(const ContextResourceRecoveryInfo& pack);

            void RecoveryContextResourceImage(const ContextResourceRecoveryInfo& pack);

            void RecoveryContextResourceImageView(const ContextResourceRecoveryInfo& pack);

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

            FreeList _bindlessIndexFreeList[3]{}; // buffer, texture_cs, textuure_sample

      private:
            std::vector<std::function<void(VkCommandBuffer)>> _commandQueue;

            entt::dense_map<uint32_t, entt::entity> _windowIdToWindow{};

            IDxcLibrary* _dxcLibrary {};

            IDxcCompiler3* _dxcCompiler {};

            IDxcUtils* _dxcUtils{};

            entt::registry _world;

      private:

            moodycamel::ConcurrentQueue<ContextResourceRecoveryInfo> _resourceRecoveryQueue{};

            std::vector<ContextResourceRecoveryInfo> _resoureceRecoveryList[3]{};

      private:
            VkRenderingInfo _frameRenderingInfo{};
            VkRect2D _frameRenderingRenderArea{};
            std::vector<VkRenderingAttachmentInfo> _frameRenderingColorAttachments{};
            std::optional<VkRenderingAttachmentInfo> _frameRenderingDepthStencilAttachment{};
            std::optional<VkRenderingAttachmentInfo> _frameRenderingDepthAttachment{};
            bool _isRenderPassOpen = false;
      };
}

//
// Created by starr on 2024/6/20.
//
#pragma once

#include <shared_mutex>

#include "Helper.h"
#include "PhysicalDevice.h"
#include "LoFiGfxDefines.h"

namespace LoFi {

      namespace Component::Gfx {
            class Buffer;
            class Buffer3F;
            class Program;
            class Texture;
            class Kernel;
            class Swapchain;
      }

      class PfxContext;

      class GfxContext {
            friend class Component::Gfx::Buffer;
            friend class Component::Gfx::Buffer3F;
            friend class Component::Gfx::Program;
            friend class Component::Gfx::Texture;
            friend class Component::Gfx::Kernel;
            friend class Component::Gfx::Swapchain;
            friend class FrameGraph;
            friend class RenderNode;

            struct SamplerCIHash {
                  std::size_t operator()(const VkSamplerCreateInfo& s) const noexcept {
                        return XXH64(&s, sizeof(VkSamplerCreateInfo), 0);
                  }
            };

            struct SamplerCIEqual {
                  std::size_t operator()(const VkSamplerCreateInfo& a, const VkSamplerCreateInfo& b) const noexcept {
                        return (memcmp(&a, &b, sizeof(VkSamplerCreateInfo)) == 0);
                  }
            };

            static GfxContext* GlobalContext;

      public:
            static GfxContext* Get() { return GlobalContext; }

            NO_COPY_MOVE_CONS(GfxContext);

            GfxContext();

            ~GfxContext();

            void Init();

            void DestroyHandle(ResourceHandle handle);

            [[nodiscard]] ResourceHandle CreateSwapChain(const GfxParamCreateSwapchain& param);

            [[nodiscard]] ResourceHandle CreateTexture2D(VkFormat format, uint32_t w, uint32_t h, const GfxParamCreateTexture2D& param = {});

            [[nodiscard]] ResourceHandle CreateBuffer(const GfxParamCreateBuffer& param);

            [[nodiscard]] ResourceHandle CreateBuffer3F(const GfxParamCreateBuffer3F& param);

            [[nodiscard]] ResourceHandle CreateProgram(const GfxParamCreateProgram& param);

            [[nodiscard]] ResourceHandle CreateProgramFromFile(const GfxParamCreateProgramFromFile& param);

            [[nodiscard]] ResourceHandle CreateKernel(ResourceHandle program, const GfxParamCreateKernel& param = {});

            [[nodiscard]] ResourceHandle CreateRenderNode(const GfxParamCreateRenderNode& param);

            [[nodiscard]] Gfx2DCanvas Create2DCanvas();

            void Destroy2DCanvas(Gfx2DCanvas canvas);

            void SetRootRenderNode(ResourceHandle node) const;

            bool SetRenderNodeWait(ResourceHandle node, const GfxInfoRenderNodeWait& param);

            bool SetKernelConstant(ResourceHandle kernel, const std::string& name, const void* data);

            bool FillKernelConstant(ResourceHandle kernel, const void* data, size_t size);

            bool SetBuffer(ResourceHandle buffer, const void* data, uint64_t size, uint64_t offset_for_3f = 0);

            bool ResizeBuffer(ResourceHandle buffer, uint64_t size);

            bool SetTexture2D(ResourceHandle texture, const void* data, uint64_t size);

            bool ResizeTexture2D(ResourceHandle texture, uint32_t w, uint32_t h);

            void SetTextureSampler(ResourceHandle texture, const VkSamplerCreateInfo& sampler_ci);

            RenderNode* GetRenderGraphNodePtr(ResourceHandle node);

            //Infos

            GfxInfoKernelLayout GetKernelLayout(ResourceHandle kernel);

            [[nodiscard]] FrameGraph* BeginFrame();

            void EndFrame();

            void GenFrame();

            [[nodiscard]] uint32_t GetTextureBindlessIndex(ResourceHandle texture);

            [[nodiscard]] uint64_t GetBufferBindlessAddress(ResourceHandle buffer);

            [[nodiscard]] void* GetBufferMappedAddress(ResourceHandle buffer);

            [[nodiscard]] FrameGraph* GetFrameGraph() const;

            [[nodiscard]] uint32_t GetCurrentFrameIndex() const;

            [[nodiscard]] bool IsValidHandle(ResourceHandle handle);

            void WaitDevice() const;

            template<class T> T* ResourceFetch(ResourceHandle handle) {
                  std::shared_lock lock(_worldRWMutex);
                  return (T*)_world.try_get<T>(handle.RHandle);
            }

            void Shutdown();

      private:
            void EnqueueBufferUpdate(ResourceHandle handle);

            void EnqueueTextureUpdate(ResourceHandle handle);

            void EnqueueBuffer3FUpdate(ResourceHandle handle);

            void StageResourceUpdate();

      private:
            void RecoveryContextResource(const Internal::ContextResourceRecoveryInfo& pack);

            void StageRecoveryContextResource();

            void RecoveryAllContextResourceImmediately();

            void RecoveryContextResourceBuffer(const Internal::ContextResourceRecoveryInfo& pack) const;

            void RecoveryContextResourceBufferView(const Internal::ContextResourceRecoveryInfo& pack) const;

            void RecoveryContextResourceImage(const Internal::ContextResourceRecoveryInfo& pack);

            void RecoveryContextResourceImageView(const Internal::ContextResourceRecoveryInfo& pack) const;

            void RecoveryContextResourcePipeline(const Internal::ContextResourceRecoveryInfo& pack) const;

            void RecoveryContextResourcePipelineLayout(const Internal::ContextResourceRecoveryInfo& pack) const;

      private:

            void WaitPreviewFramesDone();

            uint32_t MakeBindlessIndexTexture(Component::Gfx::Texture* texture, uint32_t viewIndex = 0);

            void RemoveBindlessIndexTextureImmediately(Component::Gfx::Texture* texture);

            //void PrepareSwapChainRenderTarget();

            VkFence GetCurrentFence() const;

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

            Internal::FreeList _textureBindlessIndexFreeList{}; //texture_sample, texture_cs

      private:

            entt::registry _world;

            std::shared_mutex _worldRWMutex{};

      private:
            moodycamel::ConcurrentQueue<ResourceHandle> _queueTextureUpdate{};

            moodycamel::ConcurrentQueue<ResourceHandle> _queueBufferUpdate{};

            moodycamel::ConcurrentQueue<ResourceHandle> _queueBuffer3FUpdate{};

            std::vector<ResourceHandle> _updateBuffer3FLeft{};

      private:
            moodycamel::ConcurrentQueue<Internal::ContextResourceRecoveryInfo> _resourceRecoveryQueue{};

            std::vector<Internal::ContextResourceRecoveryInfo> _resoureceRecoveryList[3]{};

      private:
            std::unique_ptr<FrameGraph> _frameGraph{};

      private: // cache
            std::vector<VkSemaphore> semaphores_wait_for{};
            std::vector<VkPipelineStageFlags> dst_stage_wait_for{};
            std::vector<VkSwapchainKHR> swap_chains{};
            std::vector<uint32_t> present_image_index{};

      private:
            entt::dense_set<PfxContext*> _2DCanvas{};

      private:
            bool _first_call = true;
      };
}

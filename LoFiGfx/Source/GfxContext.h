//
// Created by starr on 2024/6/20.
//
#pragma once

#include "Helper.h"
#include "PhysicalDevice.h"

#include "../Third/xxHash/xxh3.h"
#include "GfxComponents/Defines.h"
#include "FrameGraph.h"

namespace LoFi {

      namespace Component::Gfx {
            class Buffer;
            class Program;
            class Texture;
            class Kernel;
      }

      class GfxContext {
            friend class Component::Gfx::Buffer;
            friend class Component::Gfx::Program;
            friend class Component::Gfx::Texture;
            friend class Component::Gfx::Kernel;
            friend class FrameGraph;

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

            entt::entity CreateWindow(const char* title, uint32_t w, uint32_t h);

            [[nodiscard]] entt::entity CreateTexture2D(VkFormat format, uint32_t w, uint32_t h, uint32_t mipMapCounts = 1);

            [[nodiscard]] entt::entity CreateAATexture2D(VkFormat format, uint32_t w, uint32_t h);

            [[nodiscard]] entt::entity CreateTexture2D(const void* pixel_data, size_t size, VkFormat format, uint32_t w, uint32_t h, uint32_t mipMapCounts = 1);

            template <class T>
            [[nodiscard]] entt::entity CreateTexture2D(const std::vector<T>& data, VkFormat format, uint32_t w, uint32_t h, uint32_t mipMapCounts = 1) {
                  return CreateTexture2D((void*)data.data(), data.size() * sizeof(T), format, w, h, mipMapCounts);
            }

            [[nodiscard]] entt::entity CreateBuffer(uint64_t size, bool cpu_access = false);

            [[nodiscard]] entt::entity CreateBuffer(const void* data, uint64_t size, bool cpu_access = false);

            template <class T>
            [[nodiscard]] entt::entity CreateBuffer(const std::vector<T>& data, bool cpu_access = false) {
                  return CreateBuffer(data.data(), data.size() * sizeof(T), cpu_access);
            }

            template <class T, size_t N>
            [[nodiscard]] entt::entity CreateBuffer(const std::array<T, N>& data, bool cpu_access = false) {
                  return CreateBuffer(data.data(), N * sizeof(T), cpu_access);
            }

            [[nodiscard]] entt::entity CreateProgram(const std::vector<std::string_view>& source_codes, std::string_view config, std::string_view program_name = "default_program");

            [[nodiscard]] entt::entity CreateProgramFromFile(const std::vector<std::string_view>& source_files, std::string_view config, std::string_view program_name = "default_program");


            [[nodiscard]] entt::entity CreateKernel(entt::entity program);

            [[nodiscard]] entt::entity CreateBuffer3F(uint64_t size, bool hight_dynamic = true);

            [[nodiscard]] entt::entity CreateBuffer3F(void* data, uint64_t size, bool hight_dynamic = true);

            void SetFrameResourceData(entt::entity frame_resource, void * data, uint64_t size, uint64_t offset = 0);

            bool SetKernelConstant(entt::entity kernel, const std::string& name, void* data);

            template<class T>
            bool SetKernelConstant(entt::entity kernel, const std::string& name, T data) { return SetKernelConstant(kernel, name, (void*)&data); }

            bool FillKernelConstant(entt::entity kernel, const void* data, size_t size);

            void DestroyHandle(entt::entity);

            void UploadBuffer(entt::entity buffer, const void* data, uint64_t size);

            void ResizeBuffer(entt::entity buffer, uint64_t size);

            void UploadTexture2D(entt::entity texture, const void* data, uint64_t size);

            void* PollEvent();

            void BeginFrame();

            void EndFrame();

            void DestroyWindow(uint32_t id);

            void DestroyWindow(entt::entity window);

            void MapRenderTargetToWindow(entt::entity texture, entt::entity window);

            void CmdBeginRenderPass(const std::vector<RenderPassBeginArgument>& textures) const;

            void CmdEndRenderPass() const;

            void CmdBindKernel(entt::entity kernel) const;

            void CmdBindVertexBuffer(entt::entity buffer, uint32_t first_binding = 0, uint32_t binding_count = 1, size_t offset = 0) const;

            void CmdBindIndexBuffer(entt::entity buffer, size_t offset = 0) const;

            void CmdDraw(uint32_t vertex_count, uint32_t instance_count = 1, uint32_t first_vertex = 0, uint32_t first_instance = 0) const;

            void CmdDrawIndex(uint32_t index_count, uint32_t instance_count = 1, uint32_t first_index = 0, int32_t vertex_offset = 0, uint32_t first_instance = 0) const;

            void CmdDrawIndexedIndirect(entt::entity indirect_buffer, size_t offset, uint32_t draw_count, uint32_t stride) const;

            void CmdSetViewport(float x, float y, float w, float h, float min_depth = 0.0f, float max_depth = 1.0f) const;

            void CmdSetViewport(const VkViewport& viewport) const;

            void CmdSetViewportAuto(bool invert_y = true) const;

            void CmdSetScissor(int x, int y, uint32_t w, uint32_t h) const;

            void CmdSetScissor(VkRect2D scissor) const;

            void CmdSetScissorAuto() const;

            void CmdPushConstant(entt::entity push_constant_buffer) const;

            void BeginComputePass() const;

            void EndComputePass();

            void CmdComputeDispatch(uint32_t x, uint32_t y, uint32_t z) const;

            void SetTextureSampler(entt::entity image, const VkSamplerCreateInfo& sampler_ci);

            uint32_t GetTextureBindlessIndex(entt::entity texture);

            uint64_t GetBufferBindlessAddress(entt::entity buffer);

            void AsSampledTexure(entt::entity texture, KernelType which_kernel_use = GRAPHICS) const;

            void AsReadTexure(entt::entity texture, KernelType which_kernel_use = COMPUTE) const;

            void AsWriteTexture(entt::entity texture, KernelType which_kernel_use = COMPUTE) const;

            void AsWriteBuffer(entt::entity buffer, KernelType which_kernel_use = COMPUTE) const;

            void AsReadBuffer(entt::entity buffer, KernelType which_kernel_use = COMPUTE) const;

            uint32_t GetCurrentFrameIndex() const;

            template<class T> T* UnsafeFetch(entt::entity id) {
                  if(!_world.valid(id)) {
                        return nullptr;
                  }
                  return _world.try_get<T>(id);
            }

      private:

            void Shutdown();

            void RecoveryContextResource(const Internal::ContextResourceRecoveryInfo& pack);

            uint32_t MakeBindlessIndexTexture(entt::entity texture, uint32_t viewIndex = 0);

            void PrepareWindowRenderTarget();

            VkFence GetCurrentFence() const;

            void GoNextFrame();

            void StageRecoveryContextResource();

            void RecoveryAllContextResourceImmediately();

            void RecoveryContextResourceWindow(const Internal::ContextResourceRecoveryInfo& pack);

            void RecoveryContextResourceBuffer(const Internal::ContextResourceRecoveryInfo& pack) const;

            void RecoveryContextResourceBufferView(const Internal::ContextResourceRecoveryInfo& pack) const;

            void RecoveryContextResourceImage(const Internal::ContextResourceRecoveryInfo& pack);

            void RecoveryContextResourceImageView(const Internal::ContextResourceRecoveryInfo& pack) const;

            void RecoveryContextResourcePipeline(const Internal::ContextResourceRecoveryInfo& pack) const;

            void RecoveryContextResourcePipelineLayout(const Internal::ContextResourceRecoveryInfo& pack) const;

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

            entt::dense_map<uint32_t, entt::entity> _windowIdToWindow{};

            entt::registry _world;

      private:
            moodycamel::ConcurrentQueue<Internal::ContextResourceRecoveryInfo> _resourceRecoveryQueue{};

            std::vector<Internal::ContextResourceRecoveryInfo> _resoureceRecoveryList[3]{};

      private:
            std::unique_ptr<FrameGraph> _frameGraphs[3]{};
      };
}

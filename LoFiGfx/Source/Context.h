//
// Created by starr on 2024/6/20.
//
#pragma once

#include "Helper.h"
#include "PhysicalDevice.h"

#include "../Third/xxHash/xxh3.h"
#include "Components/Defines.h"

namespace LoFi {

      namespace Component {
            class Buffer;
            class Program;
            class Texture;
            class Kernel;
            class KernelInstance;
      }

      namespace Internal {
            enum class ContextResourceType {
                  UNKONWN,
                  WINDOW,
                  IMAGE,
                  BUFFER,
                  IMAGE_VIEW,
                  BUFFER_VIEW,
                  PIPELINE,
                  PIPELINE_LAYOUT
            };

            struct ContextResourceRecoveryInfo {
                  ContextResourceType Type = ContextResourceType::UNKONWN;
                  std::optional<size_t> Resource1{};
                  std::optional<size_t> Resource2{};
                  std::optional<size_t> Resource3{};
                  std::optional<size_t> Resource4{};
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
                  uint32_t _top{};
                  std::vector<uint32_t> _free{};
            };
      }

      struct ContextSetupParam {
            bool Debug = false;
      };

      struct LayoutVariableBindInfo {
            std::string Name;
            entt::entity Buffer;
      };

      struct RenderPassBeginArgument {
            entt::entity TextureHandle = entt::null;
            bool ClearBeforeRendering = true;
            uint32_t ViewIndex = 0;
      };

      class Context {
            friend class Component::Buffer;
            friend class Component::Program;
            friend class Component::Texture;
            friend class Component::Kernel;
            friend class Component::KernelInstance;

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

            static Context* GlobalContext;

      public:
            static Context* Get() { return GlobalContext; }

            NO_COPY_MOVE_CONS(Context);

            Context();

            ~Context();

            void Init();

            entt::entity CreateWindow(const char* title, int w, int h);

            /* [[nodiscard]] entt::entity  CreateTexture2DArray();

             [[nodiscard]] entt::entity  CreateTexture3D();

             [[nodiscard]] entt::entity  CreateTextureCube();*/

            [[nodiscard]] entt::entity CreateTexture2D(VkFormat format, uint32_t w, uint32_t h, uint32_t mipMapCounts = 1);

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

            [[nodiscard]] entt::entity CreateProgram(const std::vector<std::string_view>& source_codes, const std::string& program_name = "defualt_program");

            [[nodiscard]] entt::entity CreateKernel(entt::entity program);

            [[nodiscard]] entt::entity CreateKernelInstance(entt::entity kernel);

            void DestroyHandle(entt::entity);

            void SetBufferData(entt::entity buffer, const void* data, uint64_t size);

            void FillTexture2D(entt::entity texture, const void* data, uint64_t size);

            //void SetTexture2DData(entt::entity texture, entt::entity buffer);

            void EnqueueCommand(const std::function<void(VkCommandBuffer)>& command);

            void* PollEvent();

            void BeginFrame();

            void EndFrame();

            void DestroyWindow(uint32_t id);

            void DestroyWindow(entt::entity window);

            void MapRenderTargetToWindow(entt::entity texture, entt::entity window);

            void CmdBeginRenderPass(const std::vector<RenderPassBeginArgument>& textures);

            void CmdEndRenderPass();

            void CmdBindKernel(entt::entity kernel);

            bool BindKernelResource(entt::entity kernel_instance, const std::string& resource_name, entt::entity resource);

            template<class T> requires !std::is_pointer_v<T>
            void SetKernelParam(entt::entity frame_resource, const std::string& variable_name, const T& data) {
                  SetKernelParam(frame_resource, variable_name, &data);
            }

            template<class T> requires !std::is_pointer_v<T>
            void SetKernelParamStruct(entt::entity frame_resource, const std::string& variable_name, const T& data) {
                  SetKernelParamStruct(frame_resource, variable_name, &data);
            }

            template<class T> requires !std::is_pointer_v<T>
            void SetKernelParamMember(entt::entity frame_resource, const std::string& variable_name, const T& data) {
                  SetKernelParamMember(frame_resource, variable_name, &data);
            }

            void CmdBindVertexBuffer(entt::entity buffer, size_t offset = 0);

            void CmdDraw(uint32_t vertex_count, uint32_t instance_count = 1, uint32_t first_vertex = 0, uint32_t first_instance = 0) const;

            void CmdDrawIndex(entt::entity index_buffer, size_t offset = 0, std::optional<uint32_t> index_count = {});

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

            void RecoveryContextResource(const Internal::ContextResourceRecoveryInfo& pack);

            uint32_t MakeBindlessIndexTextureForSampler(entt::entity texture, uint32_t viewIndex = 0);

            uint32_t MakeBindlessIndexTextureForStorage(entt::entity texture, uint32_t viewIndex = 0);

            void SetTextureSampler(entt::entity image, const VkSamplerCreateInfo& sampler_ci);

            void PrepareWindowRenderTarget();

            uint32_t GetCurrentFrameIndex() const;

            VkCommandBuffer GetCurrentCommandBuffer() const;

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

            Internal::FreeList _bindlessIndexFreeList[2]{}; //texture_sample, texture_cs

      private:
            std::vector<std::function<void(VkCommandBuffer)>> _commandQueue;

            entt::dense_map<uint32_t, entt::entity> _windowIdToWindow{};

            entt::registry _world;

      private:
            moodycamel::ConcurrentQueue<Internal::ContextResourceRecoveryInfo> _resourceRecoveryQueue{};

            std::vector<Internal::ContextResourceRecoveryInfo> _resoureceRecoveryList[3]{};

      private:
            VkRect2D _frameRenderingRenderArea{};

            Component::KernelType _currentKernelType{};
            entt::entity _currentKernel{};

            bool _isRenderPassOpen = false;
      };
}

#include "GfxContext.h"
#include "PfxContext.h"

#include "Message.h"
#include "PhysicalDevice.h"
#include "FrameGraph.h"

#include "GfxComponents/Swapchain.h"
#include "GfxComponents/Buffer.h"
#include "GfxComponents/Texture.h"
#include "GfxComponents/Program.h"
#include "GfxComponents/Kernel.h"
#include "GfxComponents/Buffer3F.h"

#include <fstream>
#include <sstream>

using namespace LoFi;
using namespace LoFi::Internal;

GfxContext* GfxContext::GlobalContext = nullptr;

GfxContext::GfxContext() {
      if (GlobalContext) {
            MessageManager::Log(MessageType::Error, "Context already exists");
            throw std::runtime_error("Context already exists");
      }
      GlobalContext = this;

      semaphores_wait_for.reserve(32);
      dst_stage_wait_for.reserve(32);
      swap_chains.reserve(32);
      present_image_index.reserve(32);
      _resoureceRecoveryList[0].reserve(512);
      _resoureceRecoveryList[1].reserve(512);
      _resoureceRecoveryList[2].reserve(512);
}

GfxContext::~GfxContext() {
      Shutdown();
      GlobalContext = nullptr;
}

void GfxContext::Init() {
      volkInitialize();

      std::vector<const char*> instance_layers{};
      if (_bDebugMode) instance_layers.push_back("VK_LAYER_KHRONOS_validation");

      std::vector needed_instance_extension{
            "VK_KHR_surface",
            "VK_KHR_win32_surface"
      };

      {
            uint32_t count = 0;
            vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
            std::vector<VkExtensionProperties> extensions(count);
            vkEnumerateInstanceExtensionProperties(nullptr, &count, extensions.data());

            std::vector<const char*> missing_extensions_names{};
            for (const auto& ext : needed_instance_extension) {
                  if (std::ranges::find_if(extensions, [&ext](const auto& extension) {
                        return std::strcmp(extension.extensionName, ext) == 0;
                  }) == extensions.end()) {
                        missing_extensions_names.push_back(ext);
                  }
            }

            if (!missing_extensions_names.empty()) {
                  std::string missing_extensions{};
                  for (const auto& ext : missing_extensions_names) {
                        missing_extensions += ext;
                        missing_extensions += "\n";
                  }
                  const auto error_message = std::format("Missing instance extensions: \n{}", missing_extensions);
                  MessageManager::Log(MessageType::Error, error_message);
                  throw std::runtime_error(error_message);
            }
      }


      VkApplicationInfo app_info{};
      app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
      app_info.pApplicationName = "LoFi";
      app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
      app_info.pEngineName = "LoFi";
      app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
      app_info.apiVersion = VK_API_VERSION_1_3;

      VkInstanceCreateInfo instance_ci{};
      instance_ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
      instance_ci.pApplicationInfo = &app_info;
      instance_ci.enabledLayerCount = instance_layers.size();
      instance_ci.ppEnabledLayerNames = instance_layers.data();
      instance_ci.enabledExtensionCount = needed_instance_extension.size();
      instance_ci.ppEnabledExtensionNames = needed_instance_extension.data();

      VkInstance instance{};
      if (vkCreateInstance(&instance_ci, nullptr, &instance) != VK_SUCCESS) {
            MessageManager::Log(MessageType::Error, "Failed to create Vulkan instance");
            throw std::runtime_error("Failed to create Vulkan instance");
      }
      _instance = instance;
      volkLoadInstance(instance);

      {
            uint32_t count = 0;
            vkEnumeratePhysicalDevices(instance, &count, nullptr);
            std::vector<VkPhysicalDevice> physicalDevices(count);
            vkEnumeratePhysicalDevices(instance, &count, physicalDevices.data());

            for (const auto& physical_device : physicalDevices) {
                  PhysicalDevice ability(physical_device);

                  if (!ability.isQueueFamily0SupportAllQueue()) continue;
                  //if (!ability._accelerationStructureFeatures.accelerationStructure) continue;
                  //if (!ability._rayTracingFeatures.rayTracingPipeline) continue;
                  //if (!ability._meshShaderFeatures.meshShader) continue;

                  auto finalCard = std::format("Physical device selected: {}", ability._properties2.properties.deviceName);
                  MessageManager::Log(MessageType::Normal, finalCard);

                  _physicalDevice = physical_device;
                  _physicalDeviceAbility = ability;
                  break;
            }

            if (!_physicalDevice) {
                  const auto err = "No suitable physical device found";
                  MessageManager::Log(MessageType::Error, err);
                  throw std::runtime_error(err);
            }
      }

      {
            std::vector needed_device_extensions{

                  "VK_KHR_dynamic_rendering", // 必要
                  "VK_EXT_descriptor_indexing", // bindless

                  "VK_KHR_maintenance2",

                  "VK_KHR_swapchain", // 必要
                  "VK_KHR_get_memory_requirements2",
                  "VK_KHR_dedicated_allocation",
                  "VK_KHR_bind_memory2",
                  "VK_KHR_spirv_1_4",

                  //ray tracing
                  // "VK_KHR_deferred_host_operations",
                  //"VK_KHR_acceleration_structure",
                  //"VK_KHR_ray_tracing_pipeline",

                  //vrs
                  //"VK_KHR_fragment_shading_rate",

                  //mesh shader
                  //"VK_EXT_mesh_shader",
            };

            uint32_t count = 0;
            vkEnumerateDeviceExtensionProperties(_physicalDevice, nullptr, &count, nullptr);
            std::vector<VkExtensionProperties> extensions(count);
            vkEnumerateDeviceExtensionProperties(_physicalDevice, nullptr, &count, extensions.data());

            std::vector<const char*> missing_extensions_names{};
            for (const auto& ext : needed_device_extensions) {
                  if (std::ranges::find_if(extensions, [&ext](const auto& extension) {
                        return std::strcmp(extension.extensionName, ext) == 0;
                  }) == extensions.end()) {
                        missing_extensions_names.push_back(ext);
                  }
            }

            if (!missing_extensions_names.empty()) {
                  std::string missing_extensions{};
                  for (const auto& ext : missing_extensions_names) {
                        missing_extensions += ext;
                        missing_extensions += "\n";
                  }
                  auto error_message = std::format("Missing device extensions: \n{}", missing_extensions);
                  MessageManager::Log(MessageType::Error, error_message);
                  throw std::runtime_error(error_message);
            }

            VkDeviceQueueCreateInfo queue_ci{};
            queue_ci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_ci.queueFamilyIndex = 0;
            queue_ci.queueCount = 1;
            float queue_priority = 1.0f;
            queue_ci.pQueuePriorities = &queue_priority;

            //_physicalDeviceAbility

            // VkPhysicalDeviceMeshShaderFeaturesEXT mesh_shader_features = {
            //       .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT,
            //       .pNext = nullptr,
            //       .taskShader = false,
            //       .meshShader = false,
            //       .multiviewMeshShader = false,
            //       .primitiveFragmentShadingRateMeshShader = false,
            //       .meshShaderQueries = false
            // };

            VkPhysicalDeviceBufferDeviceAddressFeatures buffer_device_address_features{};
            buffer_device_address_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
            buffer_device_address_features.pNext = nullptr;
            buffer_device_address_features.bufferDeviceAddress = true;

            VkPhysicalDeviceSynchronization2FeaturesKHR synchronization2_features{
                  .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR,
                  .pNext = &buffer_device_address_features,
                  .synchronization2 = true,
            };

            VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamic_rendering_features = {
                  .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR,
                  .pNext = &synchronization2_features,
                  .dynamicRendering = true
            };

            VkPhysicalDeviceVulkan11Features vulkan11_features = {
                  .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
                  .pNext = &dynamic_rendering_features,
                  .storageBuffer16BitAccess = false,
                  .uniformAndStorageBuffer16BitAccess = false,
                  .storagePushConstant16 = false,
                  .storageInputOutput16 = false,
                  .multiview = false,
                  .multiviewGeometryShader = false,
                  .multiviewTessellationShader = false,
                  .variablePointersStorageBuffer = false,
                  .variablePointers = false,
                  .protectedMemory = false,
                  .samplerYcbcrConversion = false,
                  .shaderDrawParameters = true
            };

            VkPhysicalDeviceDescriptorIndexingFeatures descriptor_indexing_features = {
                  .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES,
                  .pNext = &vulkan11_features,
                  .shaderInputAttachmentArrayDynamicIndexing = true,
                  .shaderUniformTexelBufferArrayDynamicIndexing = true,
                  .shaderStorageTexelBufferArrayDynamicIndexing = true,
                  .shaderUniformBufferArrayNonUniformIndexing = true,
                  .shaderSampledImageArrayNonUniformIndexing = true,
                  .shaderStorageBufferArrayNonUniformIndexing = true,
                  .shaderStorageImageArrayNonUniformIndexing = true,
                  .shaderInputAttachmentArrayNonUniformIndexing = true,
                  .shaderUniformTexelBufferArrayNonUniformIndexing = true,
                  .shaderStorageTexelBufferArrayNonUniformIndexing = true,
                  .descriptorBindingUniformBufferUpdateAfterBind = true,
                  .descriptorBindingSampledImageUpdateAfterBind = true,
                  .descriptorBindingStorageImageUpdateAfterBind = true,
                  .descriptorBindingStorageBufferUpdateAfterBind = true,
                  .descriptorBindingUniformTexelBufferUpdateAfterBind = true,
                  .descriptorBindingStorageTexelBufferUpdateAfterBind = true,
                  .descriptorBindingUpdateUnusedWhilePending = true,
                  .descriptorBindingPartiallyBound = true,
                  .descriptorBindingVariableDescriptorCount = true,
                  .runtimeDescriptorArray = true
            };

            VkPhysicalDeviceFeatures2 features2{
                  .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
                  .pNext = &descriptor_indexing_features,
                  .features = {
                        .robustBufferAccess = false,
                        .fullDrawIndexUint32 = false,
                        .imageCubeArray = false,
                        .independentBlend = false,
                        .geometryShader = true,
                        .tessellationShader = true,
                        .sampleRateShading = false,
                        .dualSrcBlend = false,
                        .logicOp = false,
                        .multiDrawIndirect = true,
                        .drawIndirectFirstInstance = false,
                        .depthClamp = false,
                        .depthBiasClamp = false,
                        .fillModeNonSolid = true,
                        .depthBounds = false,
                        .wideLines = false,
                        .largePoints = false,
                        .alphaToOne = false,
                        .multiViewport = false,
                        .samplerAnisotropy = true,
                        .textureCompressionETC2 = false,
                        .textureCompressionASTC_LDR = false,
                        .textureCompressionBC = false,
                        .occlusionQueryPrecise = false,
                        .pipelineStatisticsQuery = false,
                        .vertexPipelineStoresAndAtomics = false,
                        .fragmentStoresAndAtomics = false,
                        .shaderTessellationAndGeometryPointSize = false,
                        .shaderImageGatherExtended = false,
                        .shaderStorageImageExtendedFormats = false,
                        .shaderStorageImageMultisample = false,
                        .shaderStorageImageReadWithoutFormat = false,
                        .shaderStorageImageWriteWithoutFormat = false,
                        .shaderUniformBufferArrayDynamicIndexing = false,
                        .shaderSampledImageArrayDynamicIndexing = false,
                        .shaderStorageBufferArrayDynamicIndexing = false,
                        .shaderStorageImageArrayDynamicIndexing = false,
                        .shaderClipDistance = false,
                        .shaderCullDistance = false,
                        .shaderFloat64 = false,
                        .shaderInt64 = false,
                        .shaderInt16 = false,
                        .shaderResourceResidency = false,
                        .shaderResourceMinLod = false,
                        .sparseBinding = false,
                        .sparseResidencyBuffer = false,
                        .sparseResidencyImage2D = false,
                        .sparseResidencyImage3D = false,
                        .sparseResidency2Samples = false,
                        .sparseResidency4Samples = false,
                        .sparseResidency8Samples = false,
                        .sparseResidency16Samples = false,
                        .sparseResidencyAliased = false,
                        .variableMultisampleRate = false,
                        .inheritedQueries = false
                  }
            };

            VkDeviceCreateInfo device_ci{};
            device_ci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            device_ci.pNext = &features2;
            device_ci.enabledExtensionCount = needed_device_extensions.size();
            device_ci.ppEnabledExtensionNames = needed_device_extensions.data();
            device_ci.queueCreateInfoCount = 1;
            device_ci.pQueueCreateInfos = &queue_ci;

            VkDevice device{};
            if (vkCreateDevice(_physicalDevice, &device_ci, nullptr, &device) != VK_SUCCESS) {
                  MessageManager::Log(MessageType::Error, "Failed to create Vulkan device");
                  throw std::runtime_error("Failed to create Vulkan device");
            }

            _device = device;
            volkLoadDevice(device);
            volkLoadPhysicalDevice(_physicalDevice);
            volkLoadEcsWorld(&_world);
      }

      {
            VmaVulkanFunctions vma_vulkan_func{};
            vma_vulkan_func.vkAllocateMemory = vkAllocateMemory;
            vma_vulkan_func.vkBindBufferMemory = vkBindBufferMemory;
            vma_vulkan_func.vkBindImageMemory = vkBindImageMemory;
            vma_vulkan_func.vkCreateBuffer = vkCreateBuffer;
            vma_vulkan_func.vkCreateImage = vkCreateImage;
            vma_vulkan_func.vkDestroyBuffer = vkDestroyBuffer;
            vma_vulkan_func.vkDestroyImage = vkDestroyImage;
            vma_vulkan_func.vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges;
            vma_vulkan_func.vkFreeMemory = vkFreeMemory;
            vma_vulkan_func.vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements;
            vma_vulkan_func.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements;
            vma_vulkan_func.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;
            vma_vulkan_func.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties;
            vma_vulkan_func.vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges;
            vma_vulkan_func.vkMapMemory = vkMapMemory;
            vma_vulkan_func.vkUnmapMemory = vkUnmapMemory;
            vma_vulkan_func.vkCmdCopyBuffer = vkCmdCopyBuffer;
            vma_vulkan_func.vkGetImageMemoryRequirements2KHR = vkGetImageMemoryRequirements2KHR;
            vma_vulkan_func.vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2KHR;
            vma_vulkan_func.vkBindBufferMemory2KHR = vkBindBufferMemory2KHR;
            vma_vulkan_func.vkBindImageMemory2KHR = vkBindImageMemory2KHR;
            vma_vulkan_func.vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2;
            vma_vulkan_func.vkGetDeviceBufferMemoryRequirements = vkGetDeviceBufferMemoryRequirements;
            vma_vulkan_func.vkGetDeviceImageMemoryRequirements = vkGetDeviceImageMemoryRequirements;

            VmaAllocatorCreateInfo vma_allocator_create_info{};
            vma_allocator_create_info.device = _device;
            vma_allocator_create_info.physicalDevice = _physicalDevice;
            vma_allocator_create_info.instance = _instance;
            vma_allocator_create_info.vulkanApiVersion = VK_API_VERSION_1_3;
            vma_allocator_create_info.pVulkanFunctions = &vma_vulkan_func;
            vma_allocator_create_info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT; // BDA!!!!


            if (vmaCreateAllocator(&vma_allocator_create_info, &_allocator) != VK_SUCCESS) {
                  MessageManager::Log(MessageType::Error, "Failed to create VMA allocator");
                  throw std::runtime_error("Failed to create VMA allocator");
            }

            volkLoadVmaAllocator(_allocator);

            vkGetDeviceQueue(_device, 0, 0, &_queue);

            VkCommandPoolCreateInfo command_pool_ci{};
            command_pool_ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            command_pool_ci.queueFamilyIndex = 0;
            command_pool_ci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

            if (vkCreateCommandPool(_device, &command_pool_ci, nullptr, &_commandPool) != VK_SUCCESS) {
                  MessageManager::Log(MessageType::Error, "Failed to create command pool");
                  throw std::runtime_error("Failed to create command pool");
            }

            VkCommandBufferAllocateInfo command_buffer_ai{};
            command_buffer_ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            command_buffer_ai.commandPool = _commandPool;
            command_buffer_ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            command_buffer_ai.commandBufferCount = 3;

            if (vkAllocateCommandBuffers(_device, &command_buffer_ai, &_commandBuffer[0]) != VK_SUCCESS) {
                  MessageManager::Log(MessageType::Error, "Failed to allocate command buffers");
                  throw std::runtime_error("Failed to allocate command buffers");
            }

            _frameGraph = std::make_unique<FrameGraph>(std::array<VkCommandBuffer, 3>{_commandBuffer[0], _commandBuffer[1], _commandBuffer[2]});
      }

      {
            VkFenceCreateInfo fence_ci{};
            fence_ci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fence_ci.flags = VK_FENCE_CREATE_SIGNALED_BIT;

            VkSemaphoreCreateInfo semaphore_ci{};
            semaphore_ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            for (int i = 0; i < 3; i++) {
                  if (vkCreateFence(_device, &fence_ci, nullptr, &_mainCommandFence[i]) != VK_SUCCESS) {
                        const auto err = "Context::Init Failed to create fence";
                        MessageManager::Log(MessageType::Error, err);
                        throw std::runtime_error(err);
                  }

                  if (vkCreateSemaphore(_device, &semaphore_ci, nullptr, &_mainCommandQueueSemaphore[i]) != VK_SUCCESS) {
                        const auto err = "Context::Init Failed to create semaphore";
                        MessageManager::Log(MessageType::Error, err);
                        throw std::runtime_error(err);
                  }
            }
      }

      {
            std::array size{
                  VkDescriptorPoolSize{
                        .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // sampler
                        .descriptorCount = 32767
                  },
                  VkDescriptorPoolSize{
                        .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                        .descriptorCount = 32767
                  }
            };


            VkDescriptorPoolCreateInfo pool_ci{};
            pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            pool_ci.maxSets = 16;
            pool_ci.pPoolSizes = size.data();
            pool_ci.poolSizeCount = size.size();
            pool_ci.flags = VkDescriptorPoolCreateFlagBits::VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT;


            if (auto res = vkCreateDescriptorPool(_device, &pool_ci, nullptr, &_descriptorPool); res != VK_SUCCESS) {
                  const auto err = std::format("Context::Init - Failed to create descriptor pool, res {}", (uint64_t)res);
                  MessageManager::Log(MessageType::Error, err);
                  throw std::runtime_error(err);
            }

            std::array binding{
                  VkDescriptorSetLayoutBinding{
                        .binding = 0,
                        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                        .descriptorCount = 32767,
                        .stageFlags = VK_SHADER_STAGE_ALL
                  },
                  VkDescriptorSetLayoutBinding{
                        .binding = 1,
                        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                        .descriptorCount = 32767,
                        .stageFlags = VK_SHADER_STAGE_ALL
                  },
            };

            std::array<VkDescriptorBindingFlags, 2> flags{
                  VkDescriptorBindingFlagBits::VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VkDescriptorBindingFlagBits::VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
                  VkDescriptorBindingFlagBits::VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VkDescriptorBindingFlagBits::VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT
            };

            VkDescriptorSetLayoutBindingFlagsCreateInfo flag_info = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO};
            flag_info.bindingCount = flags.size();
            flag_info.pBindingFlags = flags.data();

            VkDescriptorSetLayoutCreateInfo layout_ci{};
            layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layout_ci.pBindings = binding.data();
            layout_ci.bindingCount = binding.size();
            layout_ci.pNext = &flag_info;
            layout_ci.flags = VkDescriptorSetLayoutCreateFlagBits::VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;


            if (auto res = vkCreateDescriptorSetLayout(_device, &layout_ci, nullptr, &_bindlessDescriptorSetLayout); res != VK_SUCCESS) {
                  const auto err = std::format("Context::Init - Failed to create descriptor set layout, res {}", (uint64_t)res);
                  MessageManager::Log(MessageType::Error, err);
                  throw std::runtime_error(err);
            }

            VkDescriptorSetAllocateInfo alloc_ci{
                  .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                  .descriptorPool = _descriptorPool,
                  .descriptorSetCount = 1,
                  .pSetLayouts = &_bindlessDescriptorSetLayout
            };

            if (auto res = vkAllocateDescriptorSets(_device, &alloc_ci, &_bindlessDescriptorSet); res != VK_SUCCESS) {
                  const auto err = std::format("Context::Init - Failed to allocate descriptor set, res {}", (uint64_t)res);
                  MessageManager::Log(MessageType::Error, err);
                  throw std::runtime_error(err);
            }
      }

      {
            //Default sampler
            VkSamplerCreateInfo defualt_sampler_ci {
                  .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                  .pNext = nullptr,
                  .flags = 0,
                  .magFilter = VK_FILTER_LINEAR,
                  .minFilter = VK_FILTER_LINEAR,
                  .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
                  .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
                  .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
                  .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
                  .mipLodBias = 0,
                  .anisotropyEnable = true,
                  .maxAnisotropy = _physicalDeviceAbility._properties2.properties.limits.maxSamplerAnisotropy,
                  .compareEnable = false,
                  .compareOp = VK_COMPARE_OP_ALWAYS,
                  .minLod = 0,
                  .maxLod = 0,
                  .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
                  .unnormalizedCoordinates = false
            };

            if (auto res = vkCreateSampler(_device, &defualt_sampler_ci, nullptr, &_defaultSampler); res != VK_SUCCESS) {
                  auto str = std::format("Context::Init - Failed to create default sampler, res {}", (uint64_t)res);
                  MessageManager::Log(MessageType::Error, str);
                  throw std::runtime_error(str);
            }
      }


      MessageManager::Log(MessageType::Normal, "Successfully initialized LoFi context");
}

void GfxContext::Shutdown() {
      vkDeviceWaitIdle(_device);

      for(auto i : _2DCanvas) {
            delete (Gfx2DCanvas*)i;
      }
      _2DCanvas.clear();

      vkDestroyCommandPool(_device, _commandPool, nullptr);
      _frameGraph.reset();

      {
            auto view = _world.view<RenderNode>();
            _world.destroy(view.begin(), view.end());
      }

      {
            auto view = _world.view<Component::Gfx::Swapchain>();
            _world.destroy(view.begin(), view.end());
      }

      {
            auto view = _world.view<Component::Gfx::Texture>();
            _world.destroy(view.begin(), view.end());
      }

      {
            auto view = _world.view<Component::Gfx::Buffer>();
            _world.destroy(view.begin(), view.end());
      }

      {
            auto view = _world.view<Component::Gfx::Buffer3F>();
            _world.destroy(view.begin(), view.end());
      }

      {
            auto view = _world.view<Component::Gfx::Kernel>();
            _world.destroy(view.begin(), view.end());
      }

      {
            auto view = _world.view<Component::Gfx::Program>();
            _world.destroy(view.begin(), view.end());
      }

      RecoveryAllContextResourceImmediately();

      for (int i = 0; i < 3; i++) {
            vkDestroyFence(_device, _mainCommandFence[i], nullptr);
            vkDestroySemaphore(_device, _mainCommandQueueSemaphore[i], nullptr);
      }

      vkDestroyDescriptorPool(_device, _descriptorPool, nullptr);
      vkDestroyDescriptorSetLayout(_device, _bindlessDescriptorSetLayout, nullptr);

      vkDestroySampler(_device, _defaultSampler, nullptr);

      for (auto& val : _samplers | std::views::values) {
            vkDestroySampler(_device, val, nullptr);
      }

      vmaDestroyAllocator(_allocator);
      vkDestroyDevice(_device, nullptr);
      vkDestroyInstance(_instance, nullptr);
}



ResourceHandle GfxContext::CreateSwapChain(const GfxParamCreateSwapchain& param) {
      std::unique_lock lock(_worldRWMutex);
      auto id = _world.create();

      const char* resource_name = param.pResourceName;
      if(resource_name) {
            _world.emplace<Component::Gfx::ComponentResourceName>(id, std::string(resource_name));
      }

      if(param.PtrOnSwapchainNeedResizeCallback == nullptr) {
            auto err = std::format("[Context::CreateSwapChain] delegate \"PtrOnSwapchainNeedResizeCallback\" is null!.");
            if(resource_name) err += std::format(" - Name: \"{}\"", resource_name);
            MessageManager::Log(MessageType::Error, err);
            return {GfxEnumResourceType::INVALID_RESOURCE_TYPE, entt::null};
      }

      try {
            _world.emplace<Component::Gfx::Swapchain>(id, id, param);
            return {GfxEnumResourceType::SwapChain, id};
      } catch (std::exception&) {
            _world.destroy(id);
            MessageManager::Log(MessageType::Error, "[GfxContext::CreateSwapChain] Failed.");
            return {GfxEnumResourceType::INVALID_RESOURCE_TYPE, entt::null};
      }
}

ResourceHandle GfxContext::CreateTexture2D(VkFormat format, uint32_t w, uint32_t h, const GfxParamCreateTexture2D& param) {
      const char* resource_name = param.pResourceName;
      if (w == 0 || h == 0) {
            auto err = std::format("[Context::CreateTexture2D] Invalid texture size, w = {}, h = {}, create texture failed, return null.", w, h);
            if(resource_name) err += std::format(" - \"{}\"", resource_name);
            MessageManager::Log(MessageType::Error, err);
            return {GfxEnumResourceType::INVALID_RESOURCE_TYPE, entt::null};
      }

      entt::entity id = entt::null;
      Component::Gfx::Texture* ptr;

      {
            std::unique_lock lock(_worldRWMutex);
            id = _world.create();
            if(resource_name) {
                  _world.emplace<Component::Gfx::ComponentResourceName>(id, std::string(resource_name));
            }
            auto& p = _world.emplace<Component::Gfx::Texture>(id, id);
            ptr = &p;
      }


      auto is_depth_stencil = IsDepthStencilFormat(format);
      VkImageCreateInfo image_ci{};
      image_ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
      image_ci.imageType = VK_IMAGE_TYPE_2D;
      image_ci.format = format;
      image_ci.extent = VkExtent3D{(uint32_t)w, (uint32_t)h, 1};
      image_ci.mipLevels = param.MipMapCount;
      image_ci.arrayLayers = 1;
      image_ci.samples = VK_SAMPLE_COUNT_1_BIT;
      image_ci.tiling = VK_IMAGE_TILING_OPTIMAL;
      image_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
      image_ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      image_ci.flags = 0;

      if (is_depth_stencil) {
            image_ci.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
      } else {
            image_ci.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
      }

      VkImageAspectFlags as_flag = VK_IMAGE_ASPECT_COLOR_BIT;

      if (IsDepthStencilOnlyFormat(format)) {
            as_flag = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
      } else if (IsDepthOnlyFormat(format)) {
            as_flag = VK_IMAGE_ASPECT_DEPTH_BIT;
      }

      VmaAllocationCreateInfo alloc_ci{};
      alloc_ci.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

      try {
            if(!ptr->Init(image_ci, alloc_ci, param)) {
                  std::unique_lock lock(_worldRWMutex);
                  _world.destroy(id);
                  return {GfxEnumResourceType::Texture2D, id };
            } else {

                  VkImageViewCreateInfo view_ci{};
                  view_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                  view_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
                  view_ci.format = format;
                  view_ci.components.r = VK_COMPONENT_SWIZZLE_R;
                  view_ci.components.g = VK_COMPONENT_SWIZZLE_G;
                  view_ci.components.b = VK_COMPONENT_SWIZZLE_B;
                  view_ci.components.a = VK_COMPONENT_SWIZZLE_A;
                  view_ci.subresourceRange.aspectMask = as_flag;
                  view_ci.subresourceRange.baseMipLevel = 0;
                  view_ci.subresourceRange.levelCount = 1;
                  view_ci.subresourceRange.baseArrayLayer = 0;
                  view_ci.subresourceRange.layerCount = 1;

                  ptr->CreateView(view_ci);
                  if (param.MipMapCount != 1) {
                        //TODO
                  }
                  if(!is_depth_stencil) {
                        MakeBindlessIndexTexture(ptr);
                  }
                  if(param.DataSize != 0 && param.pData != nullptr) {
                        ptr->SetData(param.pData, param.DataSize);
                  }
                  return {GfxEnumResourceType::Texture2D, id };
            }
      } catch (std::exception&) {
            std::unique_lock lock(_worldRWMutex);
            _world.destroy(id);
            MessageManager::Log(MessageType::Error, "[GfxContext::CreateTexture2D] Failed.");
            return {GfxEnumResourceType::INVALID_RESOURCE_TYPE, entt::null };
      }
}


ResourceHandle GfxContext::CreateBuffer(const GfxParamCreateBuffer& param) {
      size_t size = param.DataSize;
      if (size == 0) {
            std::string err = "[Context::CreateBuffer] Invalid Size 0, Create buffer Failed.";
            if(param.pResourceName) err += std::format(" - \"{}\"", param.pResourceName);
            MessageManager::Log(MessageType::Error, err);
            return {GfxEnumResourceType::INVALID_RESOURCE_TYPE, entt::null};
      }

      entt::entity id = entt::null;
      Component::Gfx::Buffer* ptr;

      {
            std::unique_lock lock(_worldRWMutex);
            id = _world.create();
            if(const char* resource_name = param.pResourceName) {
                  _world.emplace<Component::Gfx::ComponentResourceName>(id, std::string(resource_name));
            }
            auto& p = _world.emplace<Component::Gfx::Buffer>(id, id);
            ptr = &p;
      }

      try {
            if(!ptr->Init(param)) {
                  std::unique_lock lock(_worldRWMutex);
                  _world.destroy(id);
                  return { GfxEnumResourceType::INVALID_RESOURCE_TYPE, entt::null };
            } else {
                  return { GfxEnumResourceType::Buffer, id };
            }
      } catch (std::runtime_error&) {
            std::unique_lock lock(_worldRWMutex);
            _world.destroy(id);
            return {GfxEnumResourceType::INVALID_RESOURCE_TYPE, entt::null};
      }
}

ResourceHandle GfxContext::CreateBuffer3F(const GfxParamCreateBuffer3F& param) {
      if (param.DataSize == 0) {
            std::string err = "[Context::CreateBuffer3F] Invalid Size 0, Create Buffer3F Failed.";
            if(param.pResourceName) err += std::format(" - \"{}\"", param.pResourceName);
            MessageManager::Log(MessageType::Error, err);
            return {GfxEnumResourceType::INVALID_RESOURCE_TYPE, entt::null};
      }

      entt::entity id = entt::null;
      Component::Gfx::Buffer3F* ptr;

      {
            std::unique_lock lock(_worldRWMutex);
            id = _world.create();
            if(const char* resource_name = param.pResourceName) {
                  _world.emplace<Component::Gfx::ComponentResourceName>(id, std::string(resource_name));
            }
            auto& p = _world.emplace<Component::Gfx::Buffer3F>(id, id);
            ptr = &p;
      }

      try {
            if(!ptr->Init(param)) {
                  std::unique_lock lock(_worldRWMutex);
                  _world.destroy(id);
                  return { GfxEnumResourceType::INVALID_RESOURCE_TYPE, entt::null };
            } else {
                  return { GfxEnumResourceType::Buffer3F, id };
            }
      } catch (const std::exception&) {
            std::unique_lock lock(_worldRWMutex);
            _world.destroy(id);
            MessageManager::Log(MessageType::Error, "[GfxContext::CreateBuffer3F] Failed.");
            return { GfxEnumResourceType::INVALID_RESOURCE_TYPE, entt::null };
      }
}

ResourceHandle GfxContext::CreateProgram(const GfxParamCreateProgram& param) {

      entt::entity id = entt::null;
      Component::Gfx::Program* ptr;

      {
            std::unique_lock lock(_worldRWMutex);
            id = _world.create();
            if(const char* resource_name = param.pResourceName) {
                  _world.emplace<Component::Gfx::ComponentResourceName>(id, std::string(resource_name));
            }
            ptr = &_world.emplace<Component::Gfx::Program>(id, id);
      }
      try {
            std::vector<std::string_view> source_codes{};
            for(uint32_t i = 0; i < param.countSourceCode; i++) {
                  source_codes.emplace_back(param.pSourceCodes[i]);
            }
            if(!ptr->Init(param.pResourceName, param.pConfig, source_codes)) {
                  std::unique_lock lock(_worldRWMutex);
                  _world.destroy(id);
                  return { GfxEnumResourceType::INVALID_RESOURCE_TYPE, entt::null };
            }
            return { GfxEnumResourceType::Program, id };
      } catch (const std::exception&) {
            std::unique_lock lock(_worldRWMutex);
            _world.destroy(id);
            MessageManager::Log(MessageType::Error, "[GfxContext::CreateProgram] Failed.");
            return { GfxEnumResourceType::INVALID_RESOURCE_TYPE, entt::null };
      }
}

ResourceHandle GfxContext::CreateProgramFromFile(const GfxParamCreateProgramFromFile& param) {
      std::vector<std::string_view> source_files{};
      for(uint32_t i = 0; i < param.countSourceCodeFileName; i++) {
            source_files.emplace_back(param.pSourceCodeFileNames[i]);
      }
      std::vector<std::string> codes{};
      const char* resource_name = param.pResourceName;
      for (const auto& file : source_files) {
            std::string code{};
            std::ifstream source_file(file.data());
            if(!source_file.is_open()) {
                  auto err = std::format("[Context::CreateProgramFromFile] Failed to open file {}.", file);
                  if(resource_name) err += std::format(" - \"{}\"", resource_name);
                  MessageManager::Log(MessageType::Error, err);
                  return { GfxEnumResourceType::INVALID_RESOURCE_TYPE, entt::null };
            }
            std::ostringstream oss;
            oss << source_file.rdbuf();
            codes.emplace_back(oss.str());
      }
      std::vector<const char*> codes_view{};
      for (const auto& code : codes) {
            codes_view.emplace_back(code.data());
      }

      return CreateProgram({
            .pResourceName = param.pResourceName,
            .pConfig = param.pConfig,
            .pSourceCodes = codes_view.data(),
            .countSourceCode = codes_view.size()
      });
}

ResourceHandle GfxContext::CreateKernel(ResourceHandle program, const GfxParamCreateKernel& param) {

      if(program.Type != GfxEnumResourceType::Program){
            auto err = std::format("[GfxContext::CreateKernel] Invalid Resource Type, Need a Program, but got {}.", ToStringResourceType(program.Type));
            if(param.pResourceName) err += std::format(" - Name: \"{}\"", param.pResourceName);
            MessageManager::Log(MessageType::Error, err);
            return {GfxEnumResourceType::INVALID_RESOURCE_TYPE, entt::null};
      }

      entt::entity id = entt::null;
      Component::Gfx::Kernel* kernel_ptr;
      Component::Gfx::Program* program_ptr;

      {
            std::unique_lock lock(_worldRWMutex);
            id = _world.create();

            program_ptr = _world.try_get<Component::Gfx::Program>(program.RHandle);
            if(!program_ptr) {
                  std::string err = "[GfxContext::CreateKernel] Program Handle is invalid.";
                  if(param.pResourceName) err += std::format(" - Name: \"{}\"", param.pResourceName);
                  MessageManager::Log(MessageType::Error, err);
                  return {GfxEnumResourceType::INVALID_RESOURCE_TYPE, entt::null};
            }
            if(param.pResourceName) {
                  _world.emplace<Component::Gfx::ComponentResourceName>(id, std::string(param.pResourceName));
            }
            auto& p = _world.emplace<Component::Gfx::Kernel>(id, id);
            kernel_ptr = &p;
      }

      try {
            if(!kernel_ptr->Init(program_ptr, param)) {
                  std::unique_lock lock(_worldRWMutex);
                  _world.destroy(id);
                  return { GfxEnumResourceType::INVALID_RESOURCE_TYPE, entt::null };
            } else {
                  return { GfxEnumResourceType::Kernel, id };
            }
      } catch (const std::exception&) {
            std::unique_lock lock(_worldRWMutex);
            _world.destroy(id);
            return { GfxEnumResourceType::INVALID_RESOURCE_TYPE, entt::null };
      }
}

ResourceHandle GfxContext::CreateRenderNode(const GfxParamCreateRenderNode& param) {
      if(param.pRenderNodeName == nullptr) {
            MessageManager::Log(MessageType::Error, "[GfxContext::CreateRenderNode] RenderNode's Name can't be empty or null.");
            return {GfxEnumResourceType::INVALID_RESOURCE_TYPE, entt::null};
      }
      std::string node_name = param.pRenderNodeName;
      if(node_name.empty()) {
            MessageManager::Log(MessageType::Error, "[GfxContext::CreateRenderNode] RenderNode's Name can't be empty or null.");
            return {GfxEnumResourceType::INVALID_RESOURCE_TYPE, entt::null};
      }

      std::unique_lock lock(_worldRWMutex);
      if(_frameGraph->CheckNodeExist(param.pRenderNodeName)) {
            std::string err = std::format("[GfxContext::CreateRenderNode] RenderNode with name \"{}\" already exist.", param.pRenderNodeName);
            MessageManager::Log(MessageType::Error, err);
            return {GfxEnumResourceType::INVALID_RESOURCE_TYPE, entt::null};
      } else {
            entt::entity id = entt::null;
            id = _world.create();
            RenderNode* ptr = &_world.emplace<RenderNode>(id, id, node_name);
            _frameGraph->AddNode(ptr);
            return {GfxEnumResourceType::RenderGraphNode, id};
      }
}

Gfx2DCanvas GfxContext::Create2DCanvas() {
      auto ptr = new PfxContext();
      _2DCanvas.insert(ptr);
      return (Gfx2DCanvas)ptr;
}

void GfxContext::Destroy2DCanvas(Gfx2DCanvas canvas) {
      if(_2DCanvas.contains((PfxContext*)canvas)) {
            _2DCanvas.erase((PfxContext*)canvas);
            delete (PfxContext*)canvas;
      }
}

void GfxContext::SetRootRenderNode(ResourceHandle node) const {
      return _frameGraph->SetRootNode(node);
}

bool GfxContext::SetRenderNodeWait(ResourceHandle node, const GfxInfoRenderNodeWait& param) {
      if(node.Type != GfxEnumResourceType::RenderGraphNode) {
            const auto err = std::format("[Context::SetRenderNodeWait] Invalid Resource Type, Need a RenderGraphNode, but got {}.", ToStringResourceType(node.Type));
            MessageManager::Log(MessageType::Warning, err);
            return false;
      }

      const auto ptr = ResourceFetch<RenderNode>(node);
      if (!ptr) {
            MessageManager::Log(MessageType::Warning, "[Context::SetRenderNodeWait] Invalid RenderGraphNode Handle.");
            return false;
      }
      ptr->WaitNodes(param);
      _frameGraph->SetNeedUpdate();
      return true;
}

bool GfxContext::SetKernelConstant(ResourceHandle kernel, const std::string& name, const void* data) {
      if(kernel.Type != GfxEnumResourceType::Kernel) {
            const auto err = std::format("[Context::SetKernelConstant] Invalid Resource Type, Need a Kernel, but got {}.", ToStringResourceType(kernel.Type));
            MessageManager::Log(MessageType::Warning, err);
            return false;
      }

      const auto ptr = ResourceFetch<Component::Gfx::Kernel>(kernel);
      if (!ptr) {
            const std::string err = "[Context::SetKernelConstant] Invalid Kernel Handle";
            MessageManager::Log(MessageType::Warning, err);
            return false;
      }

      return ptr->SetConstantValue(name, data);
}

bool GfxContext::FillKernelConstant(ResourceHandle kernel, const void* data, size_t size) {
      if(kernel.Type != GfxEnumResourceType::Kernel) {
            const auto err = std::format("[Context::FillKernelConstant] Invalid Resource Type, Need a Kernel, but got {}.", ToStringResourceType(kernel.Type));
            MessageManager::Log(MessageType::Warning, err);
            return false;
      }

      const auto ptr = ResourceFetch<Component::Gfx::Kernel>(kernel);
      if (!ptr) {
            std::string err = "[Context::FillKernelConstant] Invalid Kernel Handle";
            MessageManager::Log(MessageType::Warning, err);
            return false;
      }

      return ptr->FillConstantValue(data, size);
}

void GfxContext::DestroyHandle(ResourceHandle handle) {
      if(handle.Type == GfxEnumResourceType::INVALID_RESOURCE_TYPE) {
            return;
      }
      if(handle.Type == GfxEnumResourceType::RenderGraphNode) {
            std::unique_lock lock(_worldRWMutex);
            if (auto ptr = _world.try_get<RenderNode>(handle.RHandle); ptr != nullptr) {
                  _frameGraph->RemoveNode(ptr);
                  _world.destroy(handle.RHandle);
            }
      } else {
            std::unique_lock lock(_worldRWMutex);
            if (_world.valid(handle.RHandle)) {
                  _world.destroy(handle.RHandle);
            }
      }
}

bool GfxContext::SetBuffer(ResourceHandle buffer, const void* data, uint64_t size, uint64_t offset_for_3f) {
      try {
            if(buffer.Type == GfxEnumResourceType::Buffer) {
                  const auto ptr = ResourceFetch<Component::Gfx::Buffer>(buffer);
                  if (!ptr) {
                        std::string err = "[Context::UploadBuffer] Invalid Buffer Handle";
                        MessageManager::Log(MessageType::Warning, err);
                        return false;
                  }
                  ptr->SetData(data, size);
                  return true;
            } else if(buffer.Type == GfxEnumResourceType::Buffer3F) {
                  const auto ptr = ResourceFetch<Component::Gfx::Buffer3F>(buffer);
                  if (!ptr) {
                        std::string err = "[Context::UploadBuffer] Invalid Buffer3F Handle";
                        MessageManager::Log(MessageType::Warning, err);
                        return false;
                  }
                  ptr->SetData(data, size, offset_for_3f);
                  return true;
            } else {
                  const auto err = std::format("[Context::UploadBuffer] Invalid Resource Type, Need a Buffer pr Buffer3F, but got {}.", ToStringResourceType(buffer.Type));
                  MessageManager::Log(MessageType::Warning, err);
                  return false;
            }
      } catch (std::exception&) {
            MessageManager::Log(MessageType::Error, "[GfxContext::UploadBuffer] Failed.");
            return false;
      }
}

bool GfxContext::ResizeBuffer(ResourceHandle buffer, uint64_t size) {
      if(buffer.Type != GfxEnumResourceType::Buffer) {
            const auto err = std::format("[Context::ResizeBuffer] Invalid Resource Type, Need a Buffer, but got {}.", ToStringResourceType(buffer.Type));
            MessageManager::Log(MessageType::Warning, err);
            return false;
      }

      try {
            const auto ptr = ResourceFetch<Component::Gfx::Buffer>(buffer);
            if (!ptr) {
                  std::string err = "[Context::ResizeBuffer] Invalid Buffer Handle";
                  MessageManager::Log(MessageType::Warning, err);
                  return false;
            }
            return ptr->Recreate(size);
      } catch (std::exception&) {
            MessageManager::Log(MessageType::Error, "[GfxContext::ResizeBuffer] Failed.");
            return false;
      }
}

bool GfxContext::SetTexture2D(ResourceHandle texture, const void* data, uint64_t size) {
      if(texture.Type != GfxEnumResourceType::Texture2D) {
            const auto err = std::format("[Context::UploadTexture2D] Invalid Resource Type, Need a Texture2D, but got {}.", ToStringResourceType(texture.Type));
            MessageManager::Log(MessageType::Warning, err);
            return false;
      }

      try {
            const auto ptr = ResourceFetch<Component::Gfx::Texture>(texture);
            if (!ptr) {
                  std::string err = "[Context::UploadTexture2D] Invalid Texture Handle";
                  MessageManager::Log(MessageType::Warning, err);
                  return false;
            }
            ptr->SetData(data, size);
            return true;
      } catch (...) {
            MessageManager::Log(MessageType::Error, "[GfxContext::UploadTexture2D] Failed.");
            return false;
      }
}

bool GfxContext::ResizeTexture2D(ResourceHandle texture, uint32_t w, uint32_t h) {
      if(texture.Type != GfxEnumResourceType::Texture2D) {
            const auto err = std::format("[ResizeTexture2D::UploadTexture2D] Invalid Resource Type, Need a Texture2D, but got {}.", ToStringResourceType(texture.Type));
            MessageManager::Log(MessageType::Warning, err);
            return false;
      }

      try {
            const auto ptr = ResourceFetch<Component::Gfx::Texture>(texture);
            if (!ptr) {
                  std::string err = "[Context::ResizeTexture2D] Invalid Texture Handle";
                  MessageManager::Log(MessageType::Warning, err);
                  return false;
            }
            if(!ptr->Resize(w, h)) {
                  return false;
            }
            MakeBindlessIndexTexture(ptr);
            return true;
      } catch (...) {
            MessageManager::Log(MessageType::Error, "[GfxContext::ResizeTexture2D] Failed.");
            return false;
      }
}

void GfxContext::SetTextureSampler(ResourceHandle texture, const VkSamplerCreateInfo& sampler_ci) {
      if(texture.Type != GfxEnumResourceType::Texture2D) {
            const auto err = std::format("[Context::SetTextureSampler] Invalid Resource Type, Need a Texture, but got {}.", ToStringResourceType(texture.Type));
            MessageManager::Log(MessageType::Warning, err);
            return;
      }

      const auto texture_comp = ResourceFetch<Component::Gfx::Texture>(texture);
      if (!texture_comp) {
            const auto err = "[Context::SetTextureSampler] Invalid Texture Handle";
            MessageManager::Log(MessageType::Warning, err);
            return;
      }


      //TODO

      // VkSampler sampler = nullptr;
      //
      // if (const auto finder = _samplers.find(sampler_ci); finder != _samplers.end()) {
      //       sampler = finder->second;
      //       texture_comp->SetSampler(sampler);
      // } else {
      //       if (const auto result = vkCreateSampler(_device, &sampler_ci, nullptr, &sampler); result != VK_SUCCESS) {
      //             const auto err = std::format("Context::SetTextureSampler - Failed to create sampler, error code: {}.", ToStringVkResult(result));
      //             MessageManager::Log(MessageType::Error, err);
      //             throw std::runtime_error(err);
      //       }
      //       _samplers.emplace(sampler_ci, sampler);
      // }
}

RenderNode* GfxContext::GetRenderGraphNodePtr(ResourceHandle node) {
      if(node.Type != GfxEnumResourceType::RenderGraphNode) {
            const auto err = std::format("[Context::GetRenderGraphNodePtr] Invalid Resource Type, Need a RenderGraphNode, but got {}.", ToStringResourceType(node.Type));
            MessageManager::Log(MessageType::Warning, err);
            return nullptr;
      }

      const auto rdg_node = ResourceFetch<RenderNode>(node);
      if (!rdg_node) {
            const auto err = "[Context::GetRenderGraphNodePtr] Invalid RenderGraphNode Handle";
            MessageManager::Log(MessageType::Warning, err);
            return nullptr;
      }
      return  rdg_node;
}

GfxInfoKernelLayout GfxContext::GetKernelLayout(ResourceHandle kernel) {

      if(kernel.Type != GfxEnumResourceType::Kernel) {
            const auto err = std::format("[Context::GetKernelLayout] Invalid Resource Type, Need a Kernel, but got {}.", ToStringResourceType(kernel.Type));
            MessageManager::Log(MessageType::Warning, err);
            return {};
      }

      const auto texture_comp = ResourceFetch<Component::Gfx::Kernel>(kernel);
      if (!texture_comp) {
            const auto err = "[Context::GetKernelLayout] Invalid Kernel Handle";
            MessageManager::Log(MessageType::Warning, err);
            return {};
      }

      return texture_comp->GetLayout();
}

// FrameGraph* GfxContext::BeginFrame() {
//       PrepareSwapChainRenderTarget();
//
//       _frameGraphs[GetCurrentFrameIndex()]->BeginFrame();
//       auto cmd = _frameGraphs[GetCurrentFrameIndex()]->GetCommandBuffer();
//
//       Component::Gfx::Buffer::UpdateAll(cmd);
//       Component::Gfx::Buffer3F::UpdateAll();
//       Component::Gfx::Texture::UpdateAll(cmd);
//
//       _world.view<Component::Gfx::Swapchain>().each([&](auto entity, Component::Gfx::Swapchain& swapchain) {
//             swapchain.BeginFrame(cmd);
//       });
//
//       return _frameGraphs[GetCurrentFrameIndex()].get();
// }
//
// void GfxContext::EndFrame() {
//       auto cmd_buf = _frameGraphs[GetCurrentFrameIndex()]->GetCommandBuffer();
//
//
//       _world.view<Component::Gfx::Swapchain>().each([&](auto entity, auto& swapchain) {
//             swapchain.EndFrame(cmd_buf);
//
//             semaphores_wait_for.push_back(swapchain.GetCurrentSemaphore());
//             dst_stage_wait_for.push_back(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
//
//             swap_chains.push_back(swapchain.GetSwapchain());
//             present_image_index.push_back(swapchain.GetCurrentRenderTargetIndex());
//       });
//
//       _frameGraphs[GetCurrentFrameIndex()]->EndFrame();
//
//       VkSubmitInfo vk_submit_info{};
//       VkCommandBuffer buffers[] = {cmd_buf};
//       vk_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
//       vk_submit_info.commandBufferCount = 1;
//       vk_submit_info.pCommandBuffers = buffers;
//       vk_submit_info.pWaitSemaphores = semaphores_wait_for.data();
//       vk_submit_info.waitSemaphoreCount = semaphores_wait_for.size();
//       vk_submit_info.pWaitDstStageMask = dst_stage_wait_for.data();
//       vk_submit_info.pSignalSemaphores = &_mainCommandQueueSemaphore[GetCurrentFrameIndex()];
//       vk_submit_info.signalSemaphoreCount = 1;
//
//       if (vkQueueSubmit(_queue, 1, &vk_submit_info, GetCurrentFence()) != VK_SUCCESS) {
//             const auto err = "Context::EndFrame Failed to submit command buffer";
//             MessageManager::Log(MessageType::Error, err);
//             throw std::runtime_error(err);
//       }
//
//       VkPresentInfoKHR present_info{};
//       present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
//       present_info.waitSemaphoreCount = 1;
//       present_info.pWaitSemaphores = &_mainCommandQueueSemaphore[GetCurrentFrameIndex()];
//       present_info.pImageIndices = present_image_index.data();
//       present_info.pSwapchains = swap_chains.data();
//       present_info.swapchainCount = (uint32_t)swap_chains.size();
//
//       auto res = vkQueuePresentKHR(_queue, &present_info);
//       if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR) {
//
//       } else if (res != VK_SUCCESS) {
//             const auto err = "frame_end:Failed to present";
//             MessageManager::Log(MessageType::Error, err);
//             throw std::runtime_error(err);
//       }
//
//       StageRecoveryContextResource();
//
//       GoNextFrame();
// }

void GfxContext::GenFrame() {
      if(_first_call) {
            auto fence = GetCurrentFence();
            vkWaitForFences(_device, 1, &fence, true, UINT64_MAX);
            vkResetFences(_device, 1, &fence);
            _first_call = false;
      }
      //Wait pre 3 frame done
      //WaitPreviewFramesDone();
      {
            std::shared_lock lock(_worldRWMutex);

            //Get Frame State
            auto current_frame_index = GetCurrentFrameIndex();
            VkCommandBuffer current_cmd = _commandBuffer[current_frame_index];
            auto swapchain_view = _world.view<Component::Gfx::Swapchain>();

            //Prepare CommandBuffer
            if (vkResetCommandBuffer(current_cmd, 0) != VK_SUCCESS) {
                  const auto err = "FrameGraph::Reset Failed to reset command buffer";
                  MessageManager::Log(MessageType::Error, err);
                  throw std::runtime_error(err);
            }

            VkCommandBufferBeginInfo begin_info{};
            begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            if (vkBeginCommandBuffer(current_cmd, &begin_info) != VK_SUCCESS) {
                  const auto err = "FrameGraph::BeginFrame Failed to begin command buffer";
                  MessageManager::Log(MessageType::Error, err);
                  throw std::runtime_error(err);
            }

            //Do GPU Resource Update
            StageResourceUpdate();

            //Gen Commands
            _frameGraph->GenFrameGraph(current_frame_index);

            //Prepare To Present
            semaphores_wait_for.clear();
            dst_stage_wait_for.clear();
            swap_chains.clear();
            present_image_index.clear();

            swapchain_view.each([&](auto entity, const Component::Gfx::Swapchain& swapchain) {
                  swapchain.PresentBarrier(current_cmd);

                  semaphores_wait_for.push_back(swapchain.GetCurrentSemaphore());
                  dst_stage_wait_for.push_back(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

                  swap_chains.push_back(swapchain.GetSwapchain());
                  present_image_index.push_back(swapchain.GetCurrentRenderTargetIndex());

                  uint32_t t = swapchain.GetCurrentSemaphoreIndex();;
            });

            if (const auto res = vkEndCommandBuffer(current_cmd); res != VK_SUCCESS) {
                  const auto err = std::format("[GfxContext::GenFrame] vkEndCommandBuffer Failed, return {}, at frame {}.", ToStringVkResult(res), current_frame_index);
                  MessageManager::Log(MessageType::Error, err);
                  throw std::runtime_error(err);
            }

            VkSubmitInfo vk_submit_info{};
            VkCommandBuffer buffers[] = {current_cmd};
            vk_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            vk_submit_info.commandBufferCount = 1;
            vk_submit_info.pCommandBuffers = buffers;
            vk_submit_info.pWaitSemaphores = semaphores_wait_for.data();
            vk_submit_info.waitSemaphoreCount = semaphores_wait_for.size();
            vk_submit_info.pWaitDstStageMask = dst_stage_wait_for.data();
            vk_submit_info.pSignalSemaphores = &_mainCommandQueueSemaphore[GetCurrentFrameIndex()];
            vk_submit_info.signalSemaphoreCount = 1;

            if (const auto res = vkQueueSubmit(_queue, 1, &vk_submit_info, GetCurrentFence()); res != VK_SUCCESS) {
                  const auto err = std::format("[GfxContext::GenFrame] vkQueueSubmit Failed. return {}, at frame {}.", ToStringVkResult(res), current_frame_index);
                  MessageManager::Log(MessageType::Error, err);
                  throw std::runtime_error(err);
            }

            VkPresentInfoKHR present_info{};
            present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            present_info.waitSemaphoreCount = 1;
            present_info.pWaitSemaphores = &_mainCommandQueueSemaphore[GetCurrentFrameIndex()];
            present_info.pImageIndices = present_image_index.data();
            present_info.pSwapchains = swap_chains.data();
            present_info.swapchainCount = (uint32_t)swap_chains.size();

            if (const auto res = vkQueuePresentKHR(_queue, &present_info); res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR) {
                  //NeedUpdate Ignore, it will be done in Acquire
            } else if (res != VK_SUCCESS) {
                  const auto err = std::format("[GfxContext::GenFrame] vkQueuePresentKHR Failed to present. return {}, at frame {}.", ToStringVkResult(res), current_frame_index);
                  MessageManager::Log(MessageType::Error, err);
                  throw std::runtime_error(err);
            }

            //Recovery Resource
            StageRecoveryContextResource();
            //Frame Done
            GoNextFrame();

            auto fence = GetCurrentFence();
            vkWaitForFences(_device, 1, &fence, true, UINT64_MAX);
            _frameGraph->PrepareNextFrame();
            //Prepare Next Frame's Swapchain
            _world.view<Component::Gfx::Swapchain>().each([&](auto entity, Component::Gfx::Swapchain& swapchain) {
                  swapchain.AcquireNextImage();
                  uint32_t curr = swapchain.GetCurrentSemaphoreIndex();
            });

            vkResetFences(_device, 1, &fence);

      }
}



uint32_t GfxContext::MakeBindlessIndexTexture(Component::Gfx::Texture* texture, uint32_t viewIndex) {
      VkSampler sampler = texture->GetSampler();

      if (sampler == VK_NULL_HANDLE) {
            sampler = _defaultSampler;
      }

      //Sample
      VkDescriptorImageInfo image_info_sampler = {
            .sampler = sampler,
            .imageView = texture->GetView(viewIndex),
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
      };

      auto free_index = _textureBindlessIndexFreeList.Gen();

      VkWriteDescriptorSet write_sampler{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = _bindlessDescriptorSet,
            .dstBinding = 0,
            .dstArrayElement = free_index,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &image_info_sampler,
            .pBufferInfo = nullptr,
            .pTexelBufferView = nullptr
      };

      vkUpdateDescriptorSets(_device, 1, &write_sampler, 0, nullptr);

      //Storage:
      VkDescriptorImageInfo image_info_storage = {
            .sampler = sampler,
            .imageView = texture->GetView(viewIndex),
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL
      };

      VkWriteDescriptorSet write_compute{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = _bindlessDescriptorSet,
            .dstBinding = 1,
            .dstArrayElement = free_index,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .pImageInfo = &image_info_storage,
            .pBufferInfo = nullptr,
            .pTexelBufferView = nullptr
      };

      vkUpdateDescriptorSets(_device, 1, &write_compute, 0, nullptr);

      texture->SetBindlessIndex(free_index);
      auto str = std::format("[Context::MakeBindlessIndexTexture] Texture id: {}, index: {}.", (uint32_t)texture->GetHandle().RHandle, free_index);
      if(!texture->GetResourceName().empty()) str += std::format(" - Name: \"{}\"", texture->GetResourceName());
      MessageManager::Log(MessageType::Normal, str);
      return free_index;
}

void GfxContext::RemoveBindlessIndexTextureImmediately(Component::Gfx::Texture* texture) {
      if (texture->GetBindlessIndex().has_value()) {
          _textureBindlessIndexFreeList.Free(texture->GetBindlessIndex().value());
          texture->SetBindlessIndex(std::nullopt);
      }
}

uint32_t GfxContext::GetTextureBindlessIndex(ResourceHandle texture) {
      if(texture.Type == GfxEnumResourceType::Texture2D) {
            const auto texture_component = ResourceFetch<Component::Gfx::Texture>(texture);
            if (!texture_component) {
                  const auto err = "[Context::GetTextureBindlessIndex] Invalid Texture Handle";
                  MessageManager::Log(MessageType::Warning, err);
                  return 0;
            }

            if(texture_component->IsTextureFormatDepthStencilOnly()) {
                  std::string err = "[Context::GetTextureBindlessIndex] this texture is a depth stencil texture, it can't be used as bindless texture!";
                  if(const auto myname = ResourceFetch<Component::Gfx::ComponentResourceName>(texture); myname)
                        err += std::format(" - Name: \"{}\"", myname->ResourceName);
                  MessageManager::Log(MessageType::Error, err);
                  return 0;
            }

            return texture_component->GetBindlessIndex().value_or(0);
      } else if(texture.Type == GfxEnumResourceType::SwapChain) {
            const auto sp = ResourceFetch<Component::Gfx::Swapchain>(texture);
            if (!sp) {
                  const auto err = "[Context::GetTextureBindlessIndex] Invalid SwapChain Handle";
                  MessageManager::Log(MessageType::Warning, err);
                  return 0;
            }
            return sp->GetCurrentRenderTarget()->GetBindlessIndex().value();
      }  else {
            const auto err = std::format("[Context::GetTextureBindlessIndex] Invalid Resource Type, Need a Texture or SwapChain, but got {}.", ToStringResourceType(texture.Type));
            MessageManager::Log(MessageType::Error, err);
            return 0;
      }
}

uint64_t GfxContext::GetBufferBindlessAddress(ResourceHandle buffer) {
      if(buffer.Type != GfxEnumResourceType::Buffer) {
            const auto ptr = ResourceFetch<Component::Gfx::Buffer>(buffer);
            if (!ptr) {
                  const auto err = "[Context::GetBufferBindlessAddress] Invalid Texture Handle";
                  MessageManager::Log(MessageType::Warning, err);
                  return 0;
            }
            return ptr->GetBDAAddress();
      } else if(buffer.Type != GfxEnumResourceType::Buffer3F) {
            const auto ptr = ResourceFetch<Component::Gfx::Buffer3F>(buffer);
            if (!ptr) {
                  const auto err = "[Context::GetBufferBindlessAddress] Invalid Texture Handle";
                  MessageManager::Log(MessageType::Warning, err);
                  return 0;
            }
            return ptr->GetCurrentBufferBDAAddress();
      } else {
            auto err = std::format("[Context::GetBufferBindlessAddress] Invalid Resource Type, Need a Buffer or Buffer3F, but got {}.", ToStringResourceType(buffer.Type));
            MessageManager::Log(MessageType::Error, err);
            return 0;
      }
}

void* GfxContext::GetBufferMappedAddress(ResourceHandle buffer) {
      if(buffer.Type != GfxEnumResourceType::Buffer) {
            const auto err = std::format("[Context::GetBufferMappedAddress] Invalid Resource Type, Need a Buffer, but got {}.", ToStringResourceType(buffer.Type));
            MessageManager::Log(MessageType::Error, err);
            return nullptr;
      }

      const auto ptr = ResourceFetch<Component::Gfx::Buffer>(buffer);
      if (!ptr) {
            const auto err = "[Context::GetBufferMappedAddress] Invalid Texture Handle";
            MessageManager::Log(MessageType::Warning, err);
            return nullptr;
      }

      return ptr->Map();
}

FrameGraph* GfxContext::GetFrameGraph() const {
      return _frameGraph.get();
}

// void GfxContext::PrepareSwapChainRenderTarget() {
//       auto fence = GetCurrentFence();
//       vkWaitForFences(_device, 1, &fence, true, UINT64_MAX);
//
//       _world.view<Component::Gfx::Swapchain>().each([&](auto entity, auto& swapchain) {
//             swapchain.AcquireNextImage();
//       });
//
//       vkResetFences(_device, 1, &fence);
// }

uint32_t GfxContext::GetCurrentFrameIndex() const {
      return _currentCommandBufferIndex;
}

bool GfxContext::IsValidHandle(ResourceHandle handle) {
      std::shared_lock lock(_worldRWMutex);
      if(!_world.valid(handle.RHandle)) {
            return false;
      }
      switch (handle.Type) {
          case GfxEnumResourceType::Texture2D:
              return _world.any_of<Component::Gfx::Texture>(handle.RHandle);
          case GfxEnumResourceType::Buffer:
              return _world.any_of<Component::Gfx::Buffer>(handle.RHandle);
          case GfxEnumResourceType::Buffer3F:
              return _world.any_of<Component::Gfx::Buffer3F>(handle.RHandle);
          case GfxEnumResourceType::Kernel:
              return _world.any_of<Component::Gfx::Kernel>(handle.RHandle);
          case GfxEnumResourceType::Program:
              return _world.any_of<Component::Gfx::Program>(handle.RHandle);
          default:
              return false;
      }
}

void GfxContext::WaitDevice() const {
      vkDeviceWaitIdle(_device);
}

void GfxContext::EnqueueBufferUpdate(ResourceHandle handle) {
      _queueBufferUpdate.enqueue(handle);
}

void GfxContext::EnqueueTextureUpdate(ResourceHandle handle) {
      _queueTextureUpdate.enqueue(handle);
}

void GfxContext::EnqueueBuffer3FUpdate(ResourceHandle handle) {
      _queueBuffer3FUpdate.enqueue(handle);
}

void GfxContext::StageResourceUpdate() {
     // _updateBuffer3FLeft

      {
            VkCommandBuffer cmd = _commandBuffer[GetCurrentFrameIndex()];
            ResourceHandle handle{};
            std::shared_lock lock(_worldRWMutex);
            while (_queueBuffer3FUpdate.try_dequeue(handle)) {
                  if(const auto ptr = _world.try_get<Component::Gfx::Buffer3F>(handle.RHandle); ptr) {
                        if(ptr->Update()) _updateBuffer3FLeft.push_back(handle);
                  }
            }

            while (_queueBufferUpdate.try_dequeue(handle)) {
                  if(const auto ptr = _world.try_get<Component::Gfx::Buffer>(handle.RHandle); ptr) {

                        ptr->Update(cmd);
                  }
            }

            while (_queueTextureUpdate.try_dequeue(handle)) {
                  if(const auto ptr = _world.try_get<Component::Gfx::Texture>(handle.RHandle); ptr) {
                        ptr->Update(cmd);
                  }
            }
      }

      for(const auto& i : _updateBuffer3FLeft) {
            _queueBuffer3FUpdate.enqueue(i);
      }
}

VkFence GfxContext::GetCurrentFence() const {
      return _mainCommandFence[_currentCommandBufferIndex];
}

void GfxContext::GoNextFrame() {
      _currentCommandBufferIndex = (_currentCommandBufferIndex + 1) % 3;
}

void GfxContext::RecoveryContextResource(const ContextResourceRecoveryInfo& pack) {
      _resourceRecoveryQueue.enqueue(pack);
}

void GfxContext::StageRecoveryContextResource() {
      std::vector<ContextResourceRecoveryInfo>& current_list = _resoureceRecoveryList[GetCurrentFrameIndex()];
      if (!current_list.empty()) {
            for (auto& i : current_list) {
                  switch (i.Type) {
                        case ContextResourceType::BUFFER:
                              RecoveryContextResourceBuffer(i);
                              break;
                        case ContextResourceType::BUFFER_VIEW:
                              RecoveryContextResourceBufferView(i);
                              break;
                        case ContextResourceType::IMAGE:
                              RecoveryContextResourceImage(i);
                              break;
                        case ContextResourceType::IMAGE_VIEW:
                              RecoveryContextResourceImageView(i);
                              break;
                        case ContextResourceType::PIPELINE:
                              RecoveryContextResourcePipeline(i);
                              break;
                        case ContextResourceType::PIPELINE_LAYOUT:
                              RecoveryContextResourcePipelineLayout(i);
                        default: break;
                  }
            }
      }

      current_list.clear();

      ContextResourceRecoveryInfo temp{};
      while (_resourceRecoveryQueue.try_dequeue(temp)) {
            current_list.push_back(temp);
      }
}

void GfxContext::RecoveryAllContextResourceImmediately() {
      for (size_t i = GetCurrentFrameIndex(); i < GetCurrentFrameIndex() + 3; i++) {
            std::vector<ContextResourceRecoveryInfo>& current_list = _resoureceRecoveryList[i % 3];
            if (!current_list.empty()) {
                  for (auto& resource : current_list) {
                        switch (resource.Type) {
                              case ContextResourceType::BUFFER:
                                    RecoveryContextResourceBuffer(resource);
                                    break;
                              case ContextResourceType::BUFFER_VIEW:
                                    RecoveryContextResourceBufferView(resource);
                                    break;
                              case ContextResourceType::IMAGE:
                                    RecoveryContextResourceImage(resource);
                                    break;
                              case ContextResourceType::IMAGE_VIEW:
                                    RecoveryContextResourceImageView(resource);
                                    break;
                              case ContextResourceType::PIPELINE:
                                    RecoveryContextResourcePipeline(resource);
                                    break;
                              case ContextResourceType::PIPELINE_LAYOUT:
                                    RecoveryContextResourcePipelineLayout(resource);
                              default: break;
                        }
                  }
            }
            current_list.clear();
      }

      std::vector<ContextResourceRecoveryInfo>& current_list = _resoureceRecoveryList[0];

      ContextResourceRecoveryInfo temp{};
      while (_resourceRecoveryQueue.try_dequeue(temp)) {
            current_list.push_back(temp);
      }

      if (!current_list.empty()) {
            for (auto& i : current_list) {
                  switch (i.Type) {
                        case ContextResourceType::BUFFER:
                              RecoveryContextResourceBuffer(i);
                              break;
                        case ContextResourceType::BUFFER_VIEW:
                              RecoveryContextResourceBufferView(i);
                              break;
                        case ContextResourceType::IMAGE:
                              RecoveryContextResourceImage(i);
                              break;
                        case ContextResourceType::IMAGE_VIEW:
                              RecoveryContextResourceImageView(i);
                              break;
                        case ContextResourceType::PIPELINE:
                              RecoveryContextResourcePipeline(i);
                              break;
                        case ContextResourceType::PIPELINE_LAYOUT:
                              RecoveryContextResourcePipelineLayout(i);
                              break;
                        default: break;
                  }
            }
      }
}

void GfxContext::RecoveryContextResourceBuffer(const ContextResourceRecoveryInfo& pack) const {
      if (pack.Resource1.has_value() && pack.Resource2.has_value()) {
            // Image allocation
            auto buffer = (VkBuffer)pack.Resource1.value();
            auto alloc = (VmaAllocation)pack.Resource2.value();
            vmaDestroyBuffer(_allocator, buffer, alloc);

            MessageManager::Log(MessageType::Normal, "Recovery Resource Buffer");
      } else {
            auto str = std::format("Context::RecoveryContextResourceBuffer - Invalid Buffer resource");
            MessageManager::Log(MessageType::Warning, str);
      }
}

void GfxContext::RecoveryContextResourceBufferView(const ContextResourceRecoveryInfo& pack) const {
      if (pack.Resource1 != 0) {
            auto view = (VkBufferView)pack.Resource1.value();
            vkDestroyBufferView(_device, view, nullptr);

            MessageManager::Log(MessageType::Normal, "Recovery Resource BufferView");
      } else {
            auto str = std::format("Context::RecoveryContextResourceBufferView - Invalid BufferView resource");
            MessageManager::Log(MessageType::Warning, str);
      }
}

void GfxContext::RecoveryContextResourceImage(const ContextResourceRecoveryInfo& pack) {
      if (pack.Resource1.has_value() && pack.Resource2.has_value()) {
            auto image = (VkImage)pack.Resource1.value();
            auto alloc = (VmaAllocation)pack.Resource2.value();
            vmaDestroyImage(_allocator, image, alloc);

            if (pack.Resource3.has_value()) _textureBindlessIndexFreeList.Free(pack.Resource3.value());

            MessageManager::Log(MessageType::Normal, "Recovery Resource Image");
      } else {
            auto str = std::format("Context::RecoveryContextResourceImage - Invalid Image resource, {}, {}, {}, {}",
            pack.Resource1.has_value(), pack.Resource2.has_value(), pack.Resource3.has_value(), pack.Resource4.has_value());
            MessageManager::Log(MessageType::Warning, str);
      }
}

void GfxContext::RecoveryContextResourceImageView(const ContextResourceRecoveryInfo& pack) const {
      if (pack.Resource1.has_value()) {
            auto view = (VkImageView)pack.Resource1.value();
            vkDestroyImageView(_device, view, nullptr);

            MessageManager::Log(MessageType::Normal, "Recovery Resource ImageView");
      } else {
            auto str = std::format("Context::RecoveryContextResourceImageView - Invalid ImageView resource");
            MessageManager::Log(MessageType::Warning, str);
      }
}

void GfxContext::RecoveryContextResourcePipeline(const Internal::ContextResourceRecoveryInfo& pack) const {
      if (pack.Resource1.has_value()) {
            auto pipeline = (VkPipeline)pack.Resource1.value();
            vkDestroyPipeline(_device, pipeline, nullptr);
            MessageManager::Log(MessageType::Normal, "Recovery Resource Pipeline");
      } else {
            auto str = std::format("Context::RecoveryContextResourcePipeline - Invalid Pipeline resource");
            MessageManager::Log(MessageType::Warning, str);
      }
}

void GfxContext::RecoveryContextResourcePipelineLayout(const Internal::ContextResourceRecoveryInfo& pack) const {
      if (pack.Resource1.has_value()) {
            auto pipeline_layout = (VkPipelineLayout)pack.Resource1.value();
            vkDestroyPipelineLayout(_device, pipeline_layout, nullptr);
            MessageManager::Log(MessageType::Normal, "Recovery Resource PipelineLayout");
      } else {
            auto str = std::format("Context::RecoveryContextResourcePipelineLayout - Invalid PipelineLayout resource");
            MessageManager::Log(MessageType::Warning, str);
      }
}

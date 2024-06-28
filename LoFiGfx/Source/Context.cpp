#include "Context.h"
#include "Message.h"
#include "PhysicalDevice.h"

#include "SDL3/SDL.h"

using namespace LoFi;
using namespace LoFi::Internal;

Context* Context::GlobalContext = nullptr;

Context::Context() {
      if (GlobalContext) {
            MessageManager::Log(MessageType::Error, "Context already exists");
            throw std::runtime_error("Context already exists");
      }
      GlobalContext = this;
}

Context::~Context() {
      Shutdown();
      GlobalContext = nullptr;
}

void Context::Init() {
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
                  if (!ability._accelerationStructureFeatures.accelerationStructure) continue;
                  if (!ability._rayTracingFeatures.rayTracingPipeline) continue;
                  if (!ability._meshShaderFeatures.meshShader) continue;

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

            VkPhysicalDeviceMeshShaderFeaturesEXT mesh_shader_features = {
                  .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT,
                  .pNext = nullptr,
                  .taskShader = false,
                  .meshShader = false,
                  .multiviewMeshShader = false,
                  .primitiveFragmentShadingRateMeshShader = false,
                  .meshShaderQueries = false
            };

            VkPhysicalDeviceSynchronization2FeaturesKHR synchronization2_features{
                  .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR,
                  .pNext = nullptr,
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
                        .multiDrawIndirect = false,
                        .drawIndirectFirstInstance = false,
                        .depthClamp = false,
                        .depthBiasClamp = false,
                        .fillModeNonSolid = false,
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

      volkLoadEcsWorld(&_world);

      {
            std::array size{
                  //VkDescriptorPoolSize {
                  //      .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // UBO
                  //      .descriptorCount = 65535
                  //},
                  VkDescriptorPoolSize{
                        .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, // Storage Buffer
                        .descriptorCount = 65535
                  },
                  VkDescriptorPoolSize{
                        .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                        .descriptorCount = 65535
                  },
                  VkDescriptorPoolSize{
                        .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // sampler
                        .descriptorCount = 65535
                  },
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
                        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                        .descriptorCount = 32767,
                        .stageFlags = VK_SHADER_STAGE_ALL
                  },
                  VkDescriptorSetLayoutBinding{
                        .binding = 1,
                        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                        .descriptorCount = 32767,
                        .stageFlags = VK_SHADER_STAGE_ALL
                  },
                  VkDescriptorSetLayoutBinding{
                        .binding = 2,
                        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                        .descriptorCount = 32767,
                        .stageFlags = VK_SHADER_STAGE_ALL
                  },
            };

            std::array<VkDescriptorBindingFlags, 3> flags{
                  VkDescriptorBindingFlagBits::VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VkDescriptorBindingFlagBits::VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
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
            VkSamplerCreateInfo defualt_sampler_ci{
                  .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                  .pNext = nullptr,
                  .flags = 0,
                  .magFilter = VK_FILTER_LINEAR,
                  .minFilter = VK_FILTER_LINEAR,
                  .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
                  .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
                  .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
                  .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
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

void Context::Shutdown() {
      vkDeviceWaitIdle(_device);

      vkDestroyCommandPool(_device, _commandPool, nullptr);
      {
            auto view = _world.view<Component::Window, Component::Swapchain>();
            _world.destroy(view.begin(), view.end());
      }

      {
            auto view = _world.view<Component::Texture>();
            _world.destroy(view.begin(), view.end());
      }

      {
            auto view = _world.view<Component::Buffer>();
            _world.destroy(view.begin(), view.end());
      }

      {
            auto view = _world.view<Component::GraphicKernel>();
            _world.destroy(view.begin(), view.end());
      }

      {
            auto view = _world.view<Component::Program>();
            _world.destroy(view.begin(), view.end());
      }

      RecoveryAllContextResourceImmediately();

      {
            auto view = _world.view<Component::FrameResource>();
            _world.destroy(view.begin(), view.end());
      }

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

entt::entity Context::CreateWindow(const char* title, int w, int h) {
      if (w < 1 || h < 1) {
            const auto err = "Context::CreateWindow - Invalid window size";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      auto id = _world.create();
      auto& com = _world.emplace<Component::Window>(id, id, title, w, h);

      auto win_id = com.GetWindowID();
      _windowIdToWindow.emplace(win_id, id);

      return id;
}

void Context::RecoveryContextResource(const ContextResourceRecoveryInfo& pack) {
      _resourceRecoveryQueue.enqueue(pack);
}

void Context::DestroyWindow(uint32_t id) {
      ContextResourceRecoveryInfo info{
            .Type = ContextResourceType::WINDOW,
            .Resource1 = (uint64_t)id
      };
      RecoveryContextResource(info);
}

void Context::DestroyWindow(entt::entity window) {
      ContextResourceRecoveryInfo info{
            .Type = ContextResourceType::WINDOW,
            .Resource1 = (size_t)window
      };
      RecoveryContextResource(info);
}

void Context::MapRenderTargetToWindow(entt::entity texture, entt::entity window) {
      if (!_world.valid(texture)) {
            const auto err = std::format("Context::MapRenderTargetToWindow - Invalid texture entity.");
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      if (!_world.valid(window)) {
            const auto err = std::format("Context::MapRenderTargetToWindow - Invalid window entity.");
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      auto tex = _world.try_get<Component::Texture>(texture);
      if (!tex) {
            const auto err = std::format("Context::MapRenderTargetToWindow - thie entity is not texture entity.");
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      auto sp = _world.try_get<Component::Swapchain>(window);
      if (!sp) {
            const auto err = std::format("Context::MapRenderTargetToWindow - thie entity is not a renderable window entity.");
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      sp->SetMappedRenderTarget(texture);
}

void Context::CmdBindGraphicKernelToRenderPass(entt::entity kernel) {
      if (!_world.valid(kernel)) {
            const auto err = "Context::CmdBindGraphicKernel - Invalid graphics kernel entity";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      auto k = _world.try_get<Component::GraphicKernel>(kernel);
      if (!k) {
            const auto err = "Context::CmdBindGraphicKernel - this entity is not a graphics kernel";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      vkCmdBindPipeline(GetCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, k->GetPipeline());
      vkCmdBindDescriptorSets(GetCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, k->GetPipelineLayout(), 0, 1, &_bindlessDescriptorSet, 0, nullptr);
      const VkViewport viewport = VkViewport{0, (float)_frameRenderingRenderArea.extent.height, (float)_frameRenderingRenderArea.extent.width, -(float)_frameRenderingRenderArea.extent.height, 0, 1};
      vkCmdSetViewport(GetCurrentCommandBuffer(), 0, 1, &viewport);
      const VkRect2D scissor = VkRect2D{0, 0, _frameRenderingRenderArea.extent.width, _frameRenderingRenderArea.extent.height};
      vkCmdSetScissor(GetCurrentCommandBuffer(), 0, 1, &scissor);

      _currentGraphicsKernel = kernel;
}

void Context::CmdBindGraphicKernelWithFrameResourceToRenderPass(entt::entity frame_resource) {
      if (!_world.valid(frame_resource)) {
            const auto err = "Context::CmdBindGraphicKernelWithFrameResourceToRenderPass - Invalid frame resource entity";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      auto fr = _world.try_get<Component::FrameResource>(frame_resource);
      if (!fr) {
            const auto err = "Context::CmdBindGraphicKernelWithFrameResourceToRenderPass - this entity is not a frame resource";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      const auto kernel_handle = fr->GetParentGraphicsKernel();
      CmdBindGraphicKernelToRenderPass(kernel_handle);

      fr->PushBindlessInfo(GetCurrentCommandBuffer());
}

void Context::SetFrameResourceStruct(entt::entity frame_resource, const std::string& struct_name, const void* data) {
      if(!data) {
            const auto err = "Context::SetFrameResourceStruct - Invalid data, data is nullptr.";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      if (!_world.valid(frame_resource)) {
            const auto err = "Context::SetFrameResourceStruct - Invalid frame resource entity.";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      auto fr = _world.try_get<Component::FrameResource>(frame_resource);
      if (!fr) {
            const auto err = "Context::SetFrameResourceStruct - this entity is not a frame resource.";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      fr->SetStruct(struct_name, data);
}

void Context::SetFrameResourceStructMember(entt::entity frame_resource, const std::string& struct_member_name, const void* data) {
      if(!data) {
            const auto err = "Context::SetFrameResourceStruct - Invalid data, data is nullptr.";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      if (!_world.valid(frame_resource)) {
            const auto err = "Context::SetFrameResourceStruct - Invalid frame resource entity.";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      auto fr = _world.try_get<Component::FrameResource>(frame_resource);
      if (!fr) {
            const auto err = "Context::SetFrameResourceStruct - this entity is not a frame resource.";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      fr->SetStructMember(struct_member_name, data);
}

// void Context::CmdBindLayoutVariable(const std::vector<LayoutVariableBindInfo>& layout_variable_info) {
//
//       if(layout_variable_info.empty()) return;
//
//       auto kernel = _world.try_get<Component::GraphicKernel>(_currentGraphicsKernel);
//       if(!kernel) {
//             const auto err = "Context::CmdBindResourceBuffer - No graphics kernel binded";
//             MessageManager::Log(MessageType::Error, err);
//             throw std::runtime_error(err);
//       }
//
//       auto success = false;
//       for(const auto& info : layout_variable_info) {
//             auto buffer = info.Buffer;
//             const std::string& layout_variable_name = info.Name;
//
//             if (!_world.valid(buffer)) {
//                   const auto err =  std::format("Context::CmdBindResourceBuffer - Set Layout Varialbe {} failed, Invalid buffer entity", layout_variable_name);
//                   MessageManager::Log(MessageType::Error, err);
//                   throw std::runtime_error(err);
//             }
//
//             auto buf = _world.try_get<Component::Buffer>(buffer);
//             if (!buf) {
//                   const auto err = std::format("Context::CmdBindResourceBuffer - Set Layout Varialbe {} failed, entity {} is not buffer.", layout_variable_name, (uint32_t)buffer);
//                   MessageManager::Log(MessageType::Error, err);
//                   throw std::runtime_error(err);
//             }
//             auto bindless_index = buf->GetBindlessIndex();
//
//             if(!bindless_index.has_value()) {
//                   const auto err = std::format("Context::CmdBindResourceBuffer - Set Layout Varialbe {} failed, Buffer {} is not a binless buffer", layout_variable_name, (uint32_t)buffer);
//                   MessageManager::Log(MessageType::Error, err);
//                   throw std::runtime_error(err);
//             }
//
//             uint32_t bit32_index = bindless_index.value();
//             if(kernel->SetBindlessLayoutVariable(layout_variable_name, (void*)&bit32_index)) {
//                   success = true;
//             } else {
//                   const auto err = std::format("Context::CmdBindResourceBuffer - Set Layout Varialbe \"{}\" failed, this variable is not exist.", layout_variable_name);
//                   MessageManager::Log(MessageType::Error, err);
//
//                   const auto& layout_variable_names = kernel->GetVariableMap();
//                   for(const auto& i : layout_variable_names) {
//                         printf("\t\t\tVailed Name: \"%s\"\n", i.first.c_str());
//                   }
//
//             }
//       }
//
//       kernel->PushConstantBindlessLayoutVariableInfo(GetCurrentCommandBuffer());
// }

void Context::CmdBindVertexBuffer(entt::entity buffer, size_t offset) {
      if (!_world.valid(buffer)) {
            const auto err = "Context::CmdBindVertexBuffer - Invalid buffer entity";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      auto buf = _world.try_get<Component::Buffer>(buffer);
      if (!buf) {
            const auto err = "Context::CmdBindVertexBuffer - this enity is not a buffer";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      vkCmdBindVertexBuffers(GetCurrentCommandBuffer(), 0, 1, buf->GetBufferPtr(), &offset);
}

void Context::CmdDraw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance) const {
      vkCmdDraw(GetCurrentCommandBuffer(), vertex_count, instance_count, first_vertex, first_instance);
}

void Context::CmdDrawIndex(entt::entity index_buffer, size_t offset, std::optional<uint32_t> index_count) {
      if (!_world.valid(index_buffer)) {
            const auto err = "Context::CmdDrawIndex - Invalid buffer entity";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      auto ib = _world.try_get<Component::Buffer>(index_buffer);
      if (!ib) {
            const auto err = "Context::CmdDrawIndex - this entity is not a buffer";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      vkCmdBindIndexBuffer(GetCurrentCommandBuffer(), ib->GetBuffer(), offset, VK_INDEX_TYPE_UINT32);

      uint32_t max_vaild_idx_count = ib->GetSize() / sizeof(uint32_t);
      uint32_t idx_count = max_vaild_idx_count;

      if (index_count.has_value()) {
            index_count = std::min(index_count.value(), max_vaild_idx_count);
      }
      vkCmdDrawIndexed(GetCurrentCommandBuffer(), idx_count, 1, 0, 0, 0);
}

entt::entity Context::CreateTexture2D(VkFormat format, uint32_t w, uint32_t h, uint32_t mipMapCounts) {
      if (w == 0 || h == 0) {
            const auto err = std::format("Context::CreateTexture2D - Invalid texture size, w = {}, h = {}, create texture failed, return null.", w, h);
            MessageManager::Log(MessageType::Error, err);
            return entt::null;
      }

      auto id = _world.create();
      auto is_depth_stencil = IsDepthStencilFormat(format);
      VkImageCreateInfo image_ci{};
      image_ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
      image_ci.imageType = VK_IMAGE_TYPE_2D;
      image_ci.format = format;
      image_ci.extent = VkExtent3D{(uint32_t)w, (uint32_t)h, 1};
      image_ci.mipLevels = mipMapCounts;
      image_ci.arrayLayers = 1;
      image_ci.samples = VK_SAMPLE_COUNT_1_BIT;
      image_ci.tiling = VK_IMAGE_TILING_OPTIMAL;
      image_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
      image_ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      image_ci.flags = 0;

      if (is_depth_stencil) {
            image_ci.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
      } else {
            image_ci.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
      }

      VkImageAspectFlags as_flag = VK_IMAGE_ASPECT_COLOR_BIT;

      if (IsDepthStencilOnlyFormat(format)) {
            as_flag = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
      } else if (IsDepthOnlyFormat(format)) {
            as_flag = VK_IMAGE_ASPECT_DEPTH_BIT;
      }

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

      VmaAllocationCreateInfo alloc_ci{};
      alloc_ci.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

      _world.emplace<Component::Texture>(id, id, image_ci, alloc_ci).CreateView(view_ci);

      if (mipMapCounts != 1) {
            //TODO
      }

      if (!is_depth_stencil) {
            MakeBindlessIndexTextureForSampler(id);
            MakeBindlessIndexTextureForComputeKernel(id);
      }

      return id;
}


entt::entity Context::CreateBuffer(uint64_t size, bool cpu_access, bool bindless) {
      if (size == 0) {
            MessageManager::Log(MessageType::Error, "Context::CreateBuffer - Invalid buffer size, size = 0, create buffer failed, return null.");
            return entt::null;
      }

      auto id = _world.create();

      VkBufferCreateInfo buffer_ci{};
      buffer_ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
      buffer_ci.size = size;
      buffer_ci.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT |
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
      buffer_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

      VmaAllocationCreateInfo alloc_ci{};
      alloc_ci.usage = cpu_access ? VMA_MEMORY_USAGE_AUTO_PREFER_HOST : VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

      _world.emplace<Component::Buffer>(id, id, buffer_ci, alloc_ci);

      if (bindless) {
            MakeBindlessIndexBuffer(id);
      }

      return id;
}

entt::entity Context::CreateBuffer(const void* data, uint64_t size, bool cpu_access, bool bindless) {
      if (data == nullptr) {
            MessageManager::Log(MessageType::Error, "Context::CreateBuffer - Invalid data pointer, data = nullptr, create buffer failed, return null.");
            return entt::null;
      }
      if (size == 0) {
            MessageManager::Log(MessageType::Error, "Context::CreateBuffer - Invalid buffer size, size = 0, create buffer failed, return null.");
            return entt::null;
      }

      auto id = CreateBuffer(size, cpu_access, bindless);
      auto& buffer = _world.get<Component::Buffer>(id);
      buffer.SetData(data, size);
      return id;
}

entt::entity Context::CreateGraphicKernel(entt::entity program) {
      auto id = _world.create();
      auto& kernel = _world.emplace<Component::GraphicKernel>(id, id);
      bool success = kernel.CreateFromProgram(program);
      const auto& map = kernel.GetStructTable();
      const auto& map2 = kernel.GetStructMemberTable();

      for (const auto& i : map) {
            printf("%s - index: %u, size: %u\n", i.first.c_str(), i.second.Index, i.second.Size);
      }

      for (const auto& i : map2) {
            printf("%s - index: %u, size: %u, offset: %u\n", i.first.c_str(), i.second.StructIndex, i.second.Size, i.second.Offset);
      }

      if (!success) {
            _world.destroy(id);
            const auto err = "Context::CreateGraphicKernel - Failed to create graphic kernel from program";
            MessageManager::Log(MessageType::Warning, err);
            throw std::runtime_error(err);
      }

      return id;
}

entt::entity Context::CreateProgram(const std::vector<std::string_view>& source_code) {
      auto id = _world.create();
      auto& comp = _world.emplace<Component::Program>(id, id);
      if (!comp.CompileFromSourceCode("hello", source_code)) {
            _world.destroy(id);
            return entt::null;
      }
      return id;
}

entt::entity Context::CreateFrameResource(entt::entity graphics_kernel, bool is_cpu_side) {
      if (!_world.valid(graphics_kernel)) {
            const auto err = "Context::CreateFrameResource - Invalid graphics kernel entity";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      auto id = _world.create();
      _world.emplace<Component::FrameResource>(id, id, graphics_kernel, is_cpu_side);
      return id;
}

void Context::DestroyHandle(entt::entity handle) {
      if (_world.valid(handle)) {
            _world.destroy(handle);
      }
}

void Context::SetBufferData(entt::entity buffer, void* data, uint64_t size) {
      if (!_world.valid(buffer)) {
            const auto err = "Context::SetBufferData - Invalid buffer entity";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      auto buffer_component = _world.try_get<Component::Buffer>(buffer);

      if (!buffer_component) {
            const auto err = "Context::SetBufferData - this entity is not a buffer";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      buffer_component->SetData(data, size);
}

void* Context::PollEvent() {
      static SDL_Event event{};

      SDL_PollEvent(&event);

      if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
            const auto id = event.window.windowID;
            DestroyWindow(id);
      }

      if (event.type == SDL_EVENT_WINDOW_RESIZED) {
            printf("Resized");
      }

      if (_windowIdToWindow.empty()) {
            return nullptr;
      }

      return &event;
}

void Context::EnqueueCommand(const std::function<void(VkCommandBuffer)>& command) {
      _commandQueue.push_back(command);
}

void Context::BeginFrame() {
      const auto view = _world.view<Component::FrameResource, Component::TagFrameResourceChanged>();
      view.each([](entt::entity, Component::FrameResource& res) {
            res.PushResourceChanged();
      });

      _world.remove<Component::TagFrameResourceChanged>(view.begin(), view.end());

      PrepareWindowRenderTarget();
      auto cmd = GetCurrentCommandBuffer();

      if (vkResetCommandBuffer(cmd, 0) != VK_SUCCESS) {
            const auto err = "Context::BeginFrame Failed to reset command buffer";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      VkCommandBufferBeginInfo begin_info{};
      begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

      if (vkBeginCommandBuffer(cmd, &begin_info) != VK_SUCCESS) {
            const auto err = "Context::BeginFrame Failed to begin command buffer";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      for (const auto& i : _commandQueue) {
            i(cmd);
      }

      _commandQueue.clear();

      _world.view<Component::Swapchain>().each([&](auto entity, Component::Swapchain& swapchain) {
            swapchain.BeginFrame(cmd);
      });
}

void Context::EndFrame() {
      if (_isRenderPassOpen) {
            const auto err = "Context::EndFrame - Render pass is still open, close RenderPass before EndFrame!";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      auto cmd_buf = GetCurrentCommandBuffer();

      auto window_count = _windowIdToWindow.size();

      std::vector<VkSemaphore> semaphores_wait_for{};
      std::vector<VkPipelineStageFlags> dst_stage_wait_for{};
      std::vector<VkSwapchainKHR> swap_chains{};
      std::vector<uint32_t> present_image_index{};

      semaphores_wait_for.reserve(window_count);
      dst_stage_wait_for.reserve(window_count);
      swap_chains.reserve(window_count);
      present_image_index.reserve(window_count);

      _world.view<Component::Swapchain>().each([&](auto entity, auto& swapchain) {
            swapchain.EndFrame(cmd_buf);

            semaphores_wait_for.push_back(swapchain.GetCurrentSemaphore());
            dst_stage_wait_for.push_back(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

            swap_chains.push_back(swapchain.GetSwapchain());
            present_image_index.push_back(swapchain.GetCurrentRenderTargetIndex());
      });

      if (vkEndCommandBuffer(cmd_buf) != VK_SUCCESS) {
            const auto err = "Context::EndFrame Failed to end command buffer";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      VkSubmitInfo vk_submit_info{};
      VkCommandBuffer buffers[] = {cmd_buf};
      vk_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
      vk_submit_info.commandBufferCount = 1;
      vk_submit_info.pCommandBuffers = buffers;
      vk_submit_info.pWaitSemaphores = semaphores_wait_for.data();
      vk_submit_info.waitSemaphoreCount = semaphores_wait_for.size();
      vk_submit_info.pWaitDstStageMask = dst_stage_wait_for.data();
      vk_submit_info.pSignalSemaphores = &_mainCommandQueueSemaphore[GetCurrentFrameIndex()];
      vk_submit_info.signalSemaphoreCount = 1;

      if (vkQueueSubmit(_queue, 1, &vk_submit_info, GetCurrentFence()) != VK_SUCCESS) {
            const auto err = "Context::EndFrame Failed to submit command buffer";
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

      auto res = vkQueuePresentKHR(_queue, &present_info);
      if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR) {
            // _world.view<Component::Swapchain>().each([&](auto entity, auto& s) {
            //       s.CreateOrRecreateSwapChain();
            // });
      } else if (res != VK_SUCCESS) {
            const auto err = "frame_end:Failed to present";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      StageRecoveryContextResource();

      GoNextFrame();
}

void Context::CmdBeginRenderPass(const std::vector<RenderPassBeginArgument>& textures) {
      if (_isRenderPassOpen) {
            const auto err = "Context::BeginRenderPass - Render pass already open";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      if (textures.empty()) {
            const auto err = "Context::BeginRenderPass - Empty textures, create render pass failed.";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      _frameRenderingRenderArea = {};

      VkRenderingInfoKHR render_info = {
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
            .pNext = nullptr,
            .flags = 0,
            .renderArea = {},
            .layerCount = 1,
            .viewMask = 0,
            .colorAttachmentCount = 0,
            .pColorAttachments = nullptr,
            .pDepthAttachment = nullptr,
            .pStencilAttachment = nullptr
      };

      std::vector<VkRenderingAttachmentInfo> _frameRenderingColorAttachments{};
      VkRenderingAttachmentInfo _frameRenderingDepthStencilAttachment{};
      VkRenderingAttachmentInfo _frameRenderingDepthAttachment{};
      auto cmd = GetCurrentCommandBuffer();

      _frameRenderingRenderArea = {};
      for (const auto& entity : textures) {
            entt::entity handle = entity.TextureHandle;
            uint32_t view_index = entity.ViewIndex;
            bool clear = entity.ClearBeforeRendering;
            if (!_world.valid(handle)) {
                  const auto err = std::format("Context::CmdBindRenderTarget - Invalid texture entity.");
                  MessageManager::Log(MessageType::Error, err);
                  throw std::runtime_error(err);
            }

            auto texture = _world.try_get<Component::Texture>(handle);
            if (!texture) {
                  const auto err = std::format("Context::CmdBindRenderTarget - this entity is not a texture entity.");
                  MessageManager::Log(MessageType::Error, err);
                  throw std::runtime_error(err);
            }

            auto extent = texture->GetExtent();
            if (_frameRenderingRenderArea.extent.width == 0 || _frameRenderingRenderArea.extent.height == 0) {
                  _frameRenderingRenderArea = {0, 0, extent.width, extent.height};
            } else {
                  if (extent.width != _frameRenderingRenderArea.extent.width || extent.height != _frameRenderingRenderArea.extent.height) {
                        const auto str = std::format("Context::CmdBindRenderTarget - Texture size mismatch, expected {}x{}, got {}x{}", _frameRenderingRenderArea.extent.width,
                        _frameRenderingRenderArea.extent.height, extent.width, extent.height);
                        MessageManager::Log(MessageType::Error, str);
                        throw std::runtime_error(str);
                  }
            }

            if (texture->IsTextureFormatColor()) {
                  // RenderTarget:
                  auto crruent_layout = texture->GetCurrentLayout();
                  if (crruent_layout != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
                        texture->BarrierLayout(GetCurrentCommandBuffer(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, std::nullopt, std::nullopt, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
                  }

                  const VkRenderingAttachmentInfo info{
                        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                        .imageView = texture->GetView(view_index),
                        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                        .loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
                        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                        .clearValue = {.color = {0.0f, 0.0f, 0.0f, 1.0f}}
                  };

                  _frameRenderingColorAttachments.push_back(info);
                  render_info.colorAttachmentCount = _frameRenderingColorAttachments.size();
                  render_info.pColorAttachments = _frameRenderingColorAttachments.data();
            } else if (texture->IsTextureFormatDepthOnly()) {
                  // Depth:
                  auto layout = texture->GetCurrentLayout();
                  if (layout != VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) {
                        texture->BarrierLayout(GetCurrentCommandBuffer(), VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, std::nullopt, std::nullopt, VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT);
                  }

                  _frameRenderingDepthAttachment = VkRenderingAttachmentInfo{
                        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                        .imageView = texture->GetView(view_index),
                        .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                        .loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
                        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                        .clearValue = {.depthStencil = {1.0f, 0}}
                  };

                  render_info.pDepthAttachment = &_frameRenderingDepthAttachment;
                  render_info.pStencilAttachment = nullptr;
            } else if (texture->IsTextureFormatDepthStencil()) {
                  //Depth Stencil
                  auto layout = texture->GetCurrentLayout();
                  if (layout != VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
                        texture->BarrierLayout(GetCurrentCommandBuffer(), VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, std::nullopt, std::nullopt, VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT);
                  }

                  _frameRenderingDepthStencilAttachment = VkRenderingAttachmentInfo{
                        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                        .imageView = texture->GetView(view_index),
                        .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                        .loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
                        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                        .clearValue = {.depthStencil = {1.0f, 0}}
                  };

                  render_info.pDepthAttachment = &_frameRenderingDepthStencilAttachment;
                  render_info.pStencilAttachment = &_frameRenderingDepthStencilAttachment;
            } else {
                  const auto err = std::format("Context::CmdBindRenderTarget - Invalid texture format, format = {}, create render pass failed.", GetVkFormatString(texture->GetFormat()));
                  MessageManager::Log(MessageType::Error, err);
                  throw std::runtime_error(err);
            }
      }

      render_info.renderArea = _frameRenderingRenderArea;
      vkCmdBeginRenderingKHR(cmd, &render_info);
      _isRenderPassOpen = true;
}

void Context::CmdEndRenderPass() {
      if (_isRenderPassOpen) {
            vkCmdEndRendering(GetCurrentCommandBuffer());
            _isRenderPassOpen = false;
            _currentGraphicsKernel = entt::null;
      }
}

uint32_t Context::MakeBindlessIndexTextureForSampler(entt::entity texture, uint32_t viewIndex) {
      if (!_world.valid(texture)) {
            const auto err = "Context::MakeBindlessIndexTextureForSampler - Invalid texture entity";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      auto texture_component = _world.try_get<Component::Texture>(texture);

      if (!texture_component) {
            const auto err = "Context::BindTextureForSampler - Invalid texture entity";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      VkSampler sampler = texture_component->GetSampler();

      if (sampler == VK_NULL_HANDLE) {
            sampler = _defaultSampler;
      }

      VkDescriptorImageInfo image_info = {
            .sampler = sampler,
            .imageView = texture_component->GetView(viewIndex),
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
      };

      auto free_index = _bindlessIndexFreeList[1].Gen();

      VkWriteDescriptorSet write{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = _bindlessDescriptorSet,
            .dstBinding = 1,
            .dstArrayElement = free_index,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &image_info,
            .pBufferInfo = nullptr,
            .pTexelBufferView = nullptr
      };

      vkUpdateDescriptorSets(_device, 1, &write, 0, nullptr);

      texture_component->SetBindlessIndexForSampler(free_index);

      auto str = std::format("Context::MakeBindlessIndexTextureForSampler - Texture id: {}, index: {}", (uint32_t)texture_component->GetID(), free_index);
      MessageManager::Log(MessageType::Normal, str);

      return free_index;
}

uint32_t Context::MakeBindlessIndexTextureForComputeKernel(entt::entity texture, uint32_t viewIndex) {
      if (!_world.valid(texture)) {
            const auto err = "Context::MakeBindlessIndexTextureForComputeKernel - Invalid texture entity";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      auto texture_component = _world.try_get<Component::Texture>(texture);

      if (!texture_component) {
            const auto err = "Context::BindRawTexture - Invalid texture entity";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      VkSampler sampler = texture_component->GetSampler();

      if (sampler == VK_NULL_HANDLE) {
            sampler = _defaultSampler;
      }

      VkDescriptorImageInfo image_info = {
            .sampler = sampler,
            .imageView = texture_component->GetView(viewIndex),
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL
      };

      auto free_index = _bindlessIndexFreeList[2].Gen();

      VkWriteDescriptorSet write{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = _bindlessDescriptorSet,
            .dstBinding = 2,
            .dstArrayElement = free_index,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .pImageInfo = &image_info,
            .pBufferInfo = nullptr,
            .pTexelBufferView = nullptr
      };

      vkUpdateDescriptorSets(_device, 1, &write, 0, nullptr);

      texture_component->SetBindlessIndexForComputeKernel(free_index);

      auto str = std::format("Context::MakeBindlessIndexTextureForComputeKernel - Texture id: {}, index: {}", (uint32_t)texture_component->GetID(), free_index);
      MessageManager::Log(MessageType::Normal, str);

      return free_index;
}

uint32_t Context::MakeBindlessIndexBuffer(entt::entity buffer) {
      if (!_world.valid(buffer)) {
            const auto err = "Context::BindBuffer - Invalid buffer entity";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      auto buffer_component = _world.try_get<Component::Buffer>(buffer);

      if (!buffer_component) {
            const auto err = "Context::BindBuffer - Invalid buffer entity";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      VkDescriptorBufferInfo buffer_info = {
            .buffer = buffer_component->GetBuffer(),
            .offset = 0,
            .range = VK_WHOLE_SIZE
      };

      auto free_index = _bindlessIndexFreeList[0].Gen();

      VkWriteDescriptorSet write{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = _bindlessDescriptorSet,
            .dstBinding = 0,
            .dstArrayElement = free_index,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pImageInfo = nullptr,
            .pBufferInfo = &buffer_info,
            .pTexelBufferView = nullptr
      };

      vkUpdateDescriptorSets(_device, 1, &write, 0, nullptr);

      buffer_component->SetBindlessIndex(free_index);

      auto str = std::format("Context::MakeBindlessIndexBuffer - buffer id: {}, index: {}", (uint32_t)buffer_component->GetID(), free_index);
      MessageManager::Log(MessageType::Normal, str);

      return free_index;
}

void Context::SetTextureSampler(entt::entity image, const VkSamplerCreateInfo& sampler_ci) {
      auto texture = _world.try_get<Component::Texture>(image);

      if (!texture) {
            const auto err = "Context::SetTextureSampler - Invalid texture entity";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      VkSampler sampler = nullptr;

      if (const auto finder = _samplers.find(sampler_ci); finder != _samplers.end()) {
            sampler = finder->second;
            texture->SetSampler(sampler);
      } else {
            if (const auto result = vkCreateSampler(_device, &sampler_ci, nullptr, &sampler); result != VK_SUCCESS) {
                  const auto err = std::format("Context::SetTextureSampler - Failed to create sampler, error code: {}.", GetVkResultString(result));
                  MessageManager::Log(MessageType::Error, err);
                  throw std::runtime_error(err);
            }
            _samplers.emplace(sampler_ci, sampler);
      }
}

void Context::PrepareWindowRenderTarget() {
      auto fence = GetCurrentFence();
      vkWaitForFences(_device, 1, &fence, true, UINT64_MAX);

      _world.view<Component::Swapchain>().each([&](auto entity, auto& swapchain) {
            swapchain.AcquireNextImage();
      });

      vkResetFences(_device, 1, &fence);
}

uint32_t Context::GetCurrentFrameIndex() const {
      return _currentCommandBufferIndex;
}

VkCommandBuffer Context::GetCurrentCommandBuffer() const {
      return _commandBuffer[_currentCommandBufferIndex];
}

VkFence Context::GetCurrentFence() const {
      return _mainCommandFence[_currentCommandBufferIndex];
}

void Context::GoNextFrame() {
      _currentCommandBufferIndex = (_currentCommandBufferIndex + 1) % 3;
}

void Context::StageRecoveryContextResource() {
      std::vector<ContextResourceRecoveryInfo>& current_list = _resoureceRecoveryList[GetCurrentFrameIndex()];
      if (!current_list.empty()) {
            for (auto& i : current_list) {
                  switch (i.Type) {
                        case ContextResourceType::WINDOW:
                              RecoveryContextResourceWindow(i);
                              break;
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

void Context::RecoveryAllContextResourceImmediately() {
      for (size_t i = GetCurrentFrameIndex(); i < GetCurrentFrameIndex() + 3; i++) {
            std::vector<ContextResourceRecoveryInfo>& current_list = _resoureceRecoveryList[i % 3];
            if (!current_list.empty()) {
                  for (auto& resource : current_list) {
                        switch (resource.Type) {
                              case ContextResourceType::WINDOW:
                                    RecoveryContextResourceWindow(resource);
                                    break;
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
                        case ContextResourceType::WINDOW:
                              RecoveryContextResourceWindow(i);
                              break;
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

void Context::RecoveryContextResourceWindow(const ContextResourceRecoveryInfo& pack) {
      if (pack.Resource1.has_value()) {
            // entity
            entt::entity entity_id;

            auto id = (uint32_t)pack.Resource1.value();
            if (_windowIdToWindow.contains(id)) {
                  vkDeviceWaitIdle(_device);
                  entity_id = _windowIdToWindow[id];
                  if (_world.valid(entity_id) && _world.try_get<Component::Window>(entity_id)) {
                        _world.destroy(entity_id);
                  }
                  _windowIdToWindow.erase(id);
            } else {
                  entity_id = (entt::entity)pack.Resource1.value();
                  if (_world.valid(entity_id) && _world.try_get<Component::Window>(entity_id)) {
                        vkDeviceWaitIdle(_device);
                        _world.destroy(entity_id);
                  } else {
                        auto str = std::format("Context::RecoveryContextResourceWindow - Invalid Window resource");
                        MessageManager::Log(MessageType::Warning, str);
                  }
            }
            MessageManager::Log(MessageType::Normal, "Recovery Resource Window");
      } else {
            auto str = std::format("Context::RecoveryContextResourceWindow - Invalid Window resource");
            MessageManager::Log(MessageType::Warning, str);
      }
}

void Context::RecoveryContextResourceBuffer(const ContextResourceRecoveryInfo& pack) {
      if (pack.Resource1.has_value() && pack.Resource2.has_value()) {
            // Image allocation
            auto buffer = (VkBuffer)pack.Resource1.value();
            auto alloc = (VmaAllocation)pack.Resource2.value();
            vmaDestroyBuffer(_allocator, buffer, alloc);

            if (pack.Resource3.has_value())
                  _bindlessIndexFreeList[0].Free(pack.Resource3.value());

            MessageManager::Log(MessageType::Normal, "Recovery Resource Buffer");
      } else {
            auto str = std::format("Context::RecoveryContextResourceBuffer - Invalid Buffer resource");
            MessageManager::Log(MessageType::Warning, str);
      }
}

void Context::RecoveryContextResourceBufferView(const ContextResourceRecoveryInfo& pack) const {
      if (pack.Resource1 != 0) {
            auto view = (VkBufferView)pack.Resource1.value();
            vkDestroyBufferView(_device, view, nullptr);

            MessageManager::Log(MessageType::Normal, "Recovery Resource BufferView");
      } else {
            auto str = std::format("Context::RecoveryContextResourceBufferView - Invalid BufferView resource");
            MessageManager::Log(MessageType::Warning, str);
      }
}

void Context::RecoveryContextResourceImage(const ContextResourceRecoveryInfo& pack) {
      if (pack.Resource1.has_value() && pack.Resource2.has_value()) {
            auto image = (VkImage)pack.Resource1.value();
            auto alloc = (VmaAllocation)pack.Resource2.value();
            vmaDestroyImage(_allocator, image, alloc);

            if (pack.Resource3.has_value())
                  _bindlessIndexFreeList[1].Free(pack.Resource3.value());
            if (pack.Resource4.has_value())
                  _bindlessIndexFreeList[2].Free(pack.Resource4.value());

            MessageManager::Log(MessageType::Normal, "Recovery Resource Image");
      } else {
            auto str = std::format("Context::RecoveryContextResourceImage - Invalid Image resource, {}, {}, {}, {}",
            pack.Resource1.has_value(), pack.Resource2.has_value(), pack.Resource3.has_value(), pack.Resource4.has_value());
            MessageManager::Log(MessageType::Warning, str);
      }
}

void Context::RecoveryContextResourceImageView(const ContextResourceRecoveryInfo& pack) const {
      if (pack.Resource1.has_value()) {
            auto view = (VkImageView)pack.Resource1.value();
            vkDestroyImageView(_device, view, nullptr);

            MessageManager::Log(MessageType::Normal, "Recovery Resource ImageView");
      } else {
            auto str = std::format("Context::RecoveryContextResourceImageView - Invalid ImageView resource");
            MessageManager::Log(MessageType::Warning, str);
      }
}

void Context::RecoveryContextResourcePipeline(const Internal::ContextResourceRecoveryInfo& pack) {
      if (pack.Resource1.has_value()) {
            auto pipeline = (VkPipeline)pack.Resource1.value();
            vkDestroyPipeline(_device, pipeline, nullptr);
            MessageManager::Log(MessageType::Normal, "Recovery Resource Pipeline");
      } else {
            auto str = std::format("Context::RecoveryContextResourcePipeline - Invalid Pipeline resource");
            MessageManager::Log(MessageType::Warning, str);
      }
}

void Context::RecoveryContextResourcePipelineLayout(const Internal::ContextResourceRecoveryInfo& pack) const {
      if (pack.Resource1.has_value()) {
            auto pipeline_layout = (VkPipelineLayout)pack.Resource1.value();
            vkDestroyPipelineLayout(_device, pipeline_layout, nullptr);
            MessageManager::Log(MessageType::Normal, "Recovery Resource PipelineLayout");
      } else {
            auto str = std::format("Context::RecoveryContextResourcePipelineLayout - Invalid PipelineLayout resource");
            MessageManager::Log(MessageType::Warning, str);
      }
}

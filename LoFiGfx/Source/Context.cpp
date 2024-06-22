#include "Context.h"
#include "Message.h"
#include "PhysicalDevice.h"

#include <wrl/client.h>

#include "SDL3/SDL.h"

#include "../Third/dxc/dxcapi.h"

#undef CreateWindow
#undef GetWindowID

template<class T>using ComPtr = Microsoft::WRL::ComPtr<T>;
using namespace LoFi;

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

      std::vector<const char*> instance_layers{ };
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
                  auto error_message = std::format("Missing instance extensions: \n{}", missing_extensions);
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
                  std::string error_msg = "No suitable physical device found";
                  MessageManager::Log(MessageType::Error, error_msg);
                  throw std::runtime_error(error_msg);
            }
      }

      {
            std::vector needed_device_extensions{ // 必要的
                  "VK_KHR_dynamic_rendering",
                  "VK_EXT_descriptor_indexing",
                  "VK_KHR_maintenance2",

                  "VK_KHR_swapchain",
                  "VK_KHR_get_memory_requirements2",
                  "VK_KHR_dedicated_allocation",
                  "VK_KHR_bind_memory2",
                  "VK_KHR_spirv_1_4"
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

            VkDeviceCreateInfo device_ci{};
            device_ci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            device_ci.pNext = &_physicalDeviceAbility._features2;
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
                        std::string msg = "Context::Init Failed to create fence";
                        MessageManager::Log(MessageType::Error, msg);
                        throw std::runtime_error(msg);
                  }

                  if (vkCreateSemaphore(_device, &semaphore_ci, nullptr, &_mainCommandQueueSemaphore[i]) != VK_SUCCESS) {
                        std::string msg = "Context::Init Failed to create semaphore";
                        MessageManager::Log(MessageType::Error, msg);
                        throw std::runtime_error(msg);
                  }
            }
      }

      if (FAILED(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&_dxcLibrary)))) {
            MessageManager::Log(MessageType::Error, "Failed to create DXC library");
            throw std::runtime_error("Failed to create DXC library");
      }

      if (FAILED(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&_dxcCompiler)))) {
            MessageManager::Log(MessageType::Error, "Failed to create DXC compiler");
            throw std::runtime_error("Failed to create DXC compiler");
      }

      if (FAILED(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&_dxcUtils)))) {
            MessageManager::Log(MessageType::Error, "Failed to create DXC utils");
            throw std::runtime_error("Failed to create DXC utils");
      }

      volkLoadEcsWorld(&_world);

      {

            std::array size{
                  //VkDescriptorPoolSize {
                  //      .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // UBO
                  //      .descriptorCount = 65535
                  //},
                  VkDescriptorPoolSize {
                        .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, // Storage Buffer
                        .descriptorCount = 65535
                  },
                  VkDescriptorPoolSize {
                        .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                        .descriptorCount = 65535
                  },
                  VkDescriptorPoolSize {
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
                  auto msg = std::format("Context::Init - Failed to create descriptor pool, res {}", (uint64_t)res);
                  MessageManager::Log(MessageType::Error, msg);
                  throw std::runtime_error(msg);
            }

            std::array binding{
                 VkDescriptorSetLayoutBinding {
                        .binding = 0,
                        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                        .descriptorCount = 32767,
                        .stageFlags = VK_SHADER_STAGE_ALL
                 },
                 VkDescriptorSetLayoutBinding {
                        .binding = 1,
                        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                        .descriptorCount = 32767,
                        .stageFlags = VK_SHADER_STAGE_ALL
                 },
                 VkDescriptorSetLayoutBinding {
                        .binding = 2,
                        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                        .descriptorCount = 32767,
                        .stageFlags = VK_SHADER_STAGE_ALL
                 }
            };

            std::array<VkDescriptorBindingFlags, 3> flags{
                  VkDescriptorBindingFlagBits::VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VkDescriptorBindingFlagBits::VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
                  VkDescriptorBindingFlagBits::VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VkDescriptorBindingFlagBits::VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
                  VkDescriptorBindingFlagBits::VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VkDescriptorBindingFlagBits::VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT
            };

            VkDescriptorSetLayoutBindingFlagsCreateInfo flag_info = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO };
            flag_info.bindingCount = flags.size();
            flag_info.pBindingFlags = flags.data();

            VkDescriptorSetLayoutCreateInfo layout_ci{};
            layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layout_ci.pBindings = binding.data();
            layout_ci.bindingCount = binding.size();
            layout_ci.pNext = &flag_info;
            layout_ci.flags = VkDescriptorSetLayoutCreateFlagBits::VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;


            if (auto res = vkCreateDescriptorSetLayout(_device, &layout_ci, nullptr, &_bindlessDescriptorSetLayout); res != VK_SUCCESS) {
                  auto msg = std::format("Context::Init - Failed to create descriptor set layout, res {}", (uint64_t)res);
                  MessageManager::Log(MessageType::Error, msg);
                  throw std::runtime_error(msg);
            }


            VkDescriptorSetAllocateInfo alloc_ci{
                  .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                  .descriptorPool = _descriptorPool,
                  .descriptorSetCount = 1,
                  .pSetLayouts = &_bindlessDescriptorSetLayout
            };

            if (auto res = vkAllocateDescriptorSets(_device, &alloc_ci, &_bindlessDescriptorSet); res != VK_SUCCESS) {
                  auto msg = std::format("Context::Init - Failed to allocate descriptor set, res {}", (uint64_t)res);
                  MessageManager::Log(MessageType::Error, msg);
                  throw std::runtime_error(msg);
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

      for (int i = 0; i < 3; i++) {
            vkDestroyFence(_device, _mainCommandFence[i], nullptr);
            vkDestroySemaphore(_device, _mainCommandQueueSemaphore[i], nullptr);
      }

      vkDestroyDescriptorPool(_device, _descriptorPool, nullptr);
      vkDestroyDescriptorSetLayout(_device, _bindlessDescriptorSetLayout, nullptr);

      vkDestroySampler(_device, _defaultSampler, nullptr);

      for (auto i : _samplers) {
            vkDestroySampler(_device, i.second, nullptr);
      }

      vmaDestroyAllocator(_allocator);
      vkDestroyDevice(_device, nullptr);
      vkDestroyInstance(_instance, nullptr);

      _dxcUtils->Release();
      _dxcLibrary->Release();
      _dxcCompiler->Release();
}

entt::entity Context::CreateWindow(const char* title, int w, int h) {

      if (w < 1 || h < 1) {
            std::string msg = "Context::CreateWindow - Invalid window size";
            MessageManager::Log(MessageType::Error, msg);
            throw std::runtime_error(msg);
      }

      auto id = _world.create();
      auto& com = _world.emplace<Component::Window>(id, id, title, w, h);

      auto win_id = com.GetWindowID();
      _windowIdToWindow.emplace(win_id, id);

      return id;
}

void Context::DestroyWindow(uint32_t id) {

      if (_windowIdToWindow.contains(id)) {
            DestroyWindow(_windowIdToWindow[id]);
            _windowIdToWindow.erase(id);
      }
}

void Context::DestroyWindow(entt::entity window) {
      if (_world.try_get<Component::Window>(window)) {
            vkDeviceWaitIdle(_device);
            _world.destroy(window);
      }
}

entt::entity Context::CreateTexture2D(VkFormat format, int w, int h, int mipMapCounts) {
      auto id = _world.create();

      VkImageCreateInfo image_ci{};
      image_ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
      image_ci.imageType = VK_IMAGE_TYPE_2D;
      image_ci.format = VK_FORMAT_R8G8B8A8_UNORM;
      image_ci.extent = VkExtent3D{ (uint32_t)w, (uint32_t)h, 1 };
      image_ci.mipLevels = mipMapCounts;
      image_ci.arrayLayers = 1;
      image_ci.samples = VK_SAMPLE_COUNT_1_BIT;
      image_ci.tiling = VK_IMAGE_TILING_OPTIMAL;
      image_ci.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
      image_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
      image_ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      image_ci.flags = 0;



      VkImageViewCreateInfo view_ci{};
      view_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      view_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
      view_ci.format = VK_FORMAT_R8G8B8A8_UNORM;
      view_ci.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
      view_ci.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
      view_ci.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
      view_ci.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
      view_ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
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

      return id;
}


entt::entity Context::CreateBuffer(uint64_t size, bool cpu_access) {
      auto id = _world.create();

      VkBufferCreateInfo buffer_ci{};
      buffer_ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
      buffer_ci.size = size;
      buffer_ci.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
      buffer_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

      VmaAllocationCreateInfo alloc_ci{};
      alloc_ci.usage = cpu_access ? VMA_MEMORY_USAGE_AUTO_PREFER_HOST : VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

      entt::delegate<void(Context* self, const Component::Buffer*)> deleg{};

      _world.emplace<Component::Buffer>(id, id, buffer_ci, alloc_ci);

      MakeBindlessIndexBuffer(id);
      return id;
}

entt::entity Context::CreateBuffer(void* data, uint64_t size, bool cpu_access) {
      auto id = CreateBuffer(size, cpu_access);
      auto& buffer = _world.get<Component::Buffer>(id);
      buffer.SetData(data, size);
      return id;
}

entt::entity Context::CreateRenderTexture(int w, int h) {
      auto id = _world.create();

      VkImageCreateInfo image_ci{};
      image_ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
      image_ci.imageType = VK_IMAGE_TYPE_2D;
      image_ci.format = VK_FORMAT_R8G8B8A8_UNORM;
      image_ci.extent = VkExtent3D{ (uint32_t)w, (uint32_t)h, 1 };
      image_ci.mipLevels = 1;
      image_ci.arrayLayers = 1;
      image_ci.samples = VK_SAMPLE_COUNT_1_BIT;
      image_ci.tiling = VK_IMAGE_TILING_OPTIMAL;
      image_ci.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
      image_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
      image_ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      image_ci.flags = 0;

      VkImageViewCreateInfo view_ci{};
      view_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      view_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
      view_ci.format = VK_FORMAT_R8G8B8A8_UNORM;
      view_ci.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
      view_ci.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
      view_ci.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
      view_ci.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
      view_ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      view_ci.subresourceRange.baseMipLevel = 0;
      view_ci.subresourceRange.levelCount = 1;
      view_ci.subresourceRange.baseArrayLayer = 0;
      view_ci.subresourceRange.layerCount = 1;

      VmaAllocationCreateInfo alloc_ci{};
      alloc_ci.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

      _world.emplace<Component::Texture>(id, id, image_ci, alloc_ci).CreateView(view_ci);
      MakeBindlessIndexTextureForSampler(id);
      MakeBindlessIndexTextureForComputeKernel(id);
      return id;
}

entt::entity Context::CreateDepthStencil(int w, int h) {
      auto id = _world.create();

      VkImageCreateInfo image_ci{};
      image_ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
      image_ci.imageType = VK_IMAGE_TYPE_2D;
      image_ci.format = VK_FORMAT_D32_SFLOAT_S8_UINT;
      image_ci.extent = VkExtent3D{ (uint32_t)w, (uint32_t)h, 1 };
      image_ci.mipLevels = 1;
      image_ci.arrayLayers = 1;
      image_ci.samples = VK_SAMPLE_COUNT_1_BIT;
      image_ci.tiling = VK_IMAGE_TILING_OPTIMAL;
      image_ci.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
      image_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
      image_ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      image_ci.flags = 0;

      VkImageViewCreateInfo view_ci{};
      view_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      view_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
      view_ci.format = VK_FORMAT_D32_SFLOAT_S8_UINT;
      view_ci.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
      view_ci.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
      view_ci.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
      view_ci.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
      view_ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
      view_ci.subresourceRange.baseMipLevel = 0;
      view_ci.subresourceRange.levelCount = 1;
      view_ci.subresourceRange.baseArrayLayer = 0;
      view_ci.subresourceRange.layerCount = 1;

      VmaAllocationCreateInfo alloc_ci{};
      alloc_ci.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

      _world.emplace<Component::Texture>(id, id, image_ci, alloc_ci).CreateView(view_ci);
      //MakeBindlessIndexTextureForSampler(id);
      //MakeBindlessIndexTextureForComputeKernel(id);
      return id;
}

void Context::DestroyBuffer(entt::entity buffer) {

      if (!_world.valid(buffer)) {
            auto msg = "Context::DestroyBuffer - Invalid buffer entity, ignore it.";
            MessageManager::Log(MessageType::Warning, msg);
            return;
      }

      if (auto comp = _world.try_get<Component::Buffer>(buffer); comp) {
            auto bindless_index = comp->GetBindlessIndex();

            if (bindless_index.has_value()) {
                  _bindlessIndexFreeList[0].Free(bindless_index.value());
            }

            _world.destroy(buffer);
      }
}

void Context::DestroyTexture(entt::entity texture) {

      if (!_world.valid(texture)) {
            auto msg = "Context::DestroyTexture - Invalid buffer entity, ignore it.";
            MessageManager::Log(MessageType::Warning, msg);
            return;
      }

      if (auto comp = _world.try_get<Component::Texture>(texture); comp) {
            auto bindless_index_for_sampler = comp->GetBindlessIndexForSampler();
            auto bindless_index_for_compute_kernel = comp->GetBindlessIndexForComputeKernel();

            if (bindless_index_for_sampler.has_value()) {
                  _bindlessIndexFreeList[2].Free(bindless_index_for_sampler.value());
            }

            if (bindless_index_for_compute_kernel.has_value()) {
                  _bindlessIndexFreeList[1].Free(bindless_index_for_compute_kernel.value());
            }

            _world.destroy(texture);
      }
}

//void Context::DestroyTexture(Texture *texture) {
//      if(auto res = _textures.find(texture); res != _textures.end()){
//            _textures.erase(res);
//      }
//
//}


void* Context::PollEvent() {
      static SDL_Event event{};

      SDL_PollEvent(&event);

      if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
            const auto id = event.window.windowID;
            DestroyWindow(id);
      }

      if (event.type == SDL_EVENT_WINDOW_RESIZED) {
            const auto id = event.window.windowID;
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
      PrepareWindowRenderTarget();
      auto cmd = GetCurrentCommandBuffer();

      if (vkResetCommandBuffer(cmd, 0) != VK_SUCCESS) {
            std::string msg = "Context::BeginFrame Failed to reset command buffer";
            MessageManager::Log(MessageType::Error, msg);
            throw std::runtime_error(msg);
      }

      VkCommandBufferBeginInfo begin_info{};
      begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

      if (vkBeginCommandBuffer(cmd, &begin_info) != VK_SUCCESS) {
            std::string msg = "Context::BeginFrame Failed to begin command buffer";
            MessageManager::Log(MessageType::Error, msg);
            throw std::runtime_error(msg);
      }

      for (const auto& i : _commandQueue) {
            i(cmd);
      }

      _commandQueue.clear();

      //VkRenderingAttachmentInfoKHR color_attachment_info = vkb::initializers::rendering_attachment_info();
      //color_attachment_info.imageView = swapchain_buffers[i].view;        // color_attachment.image_view;


      //VkRenderingAttachmentInfoKHR depth_attachment_info = vkb::initializers::rendering_attachment_info();
      //depth_attachment_info.imageView = depth_stencil.view;


      auto render_area = VkRect2D{ VkOffset2D{}, VkExtent2D{3, 5} };
      VkRenderingInfoKHR render_info = {
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
            .pNext = nullptr,
            .flags = 0,
            .renderArea = render_area,
            .layerCount = 1,
            .viewMask = 0,
            .colorAttachmentCount = 0,
            .pColorAttachments = nullptr,
            .pDepthAttachment = nullptr,
            .pStencilAttachment = nullptr
      };

      vkCmdBeginRenderingKHR(cmd, &render_info);

      vkCmdEndRendering(cmd);
}

void Context::EndFrame() {

      auto cmd_buf = GetCurrentCommandBuffer();

      auto window_count = _windowIdToWindow.size();

      std::vector<VkImageMemoryBarrier2> barriers{};
      barriers.reserve(window_count);

      _world.view<Component::Swapchain>().each([&](auto entity, auto& swapchain) {
            barriers.push_back(swapchain.GenerateCurrentRenderTargetBarrier());
      });

      VkDependencyInfoKHR window_bgin_barrier = {
           .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR,
           .imageMemoryBarrierCount = (uint32_t)barriers.size(),
           .pImageMemoryBarriers = barriers.data()
      };

      vkCmdPipelineBarrier2(cmd_buf, &window_bgin_barrier);

      barriers.clear();

      _world.view<Component::Swapchain>().each([&](auto entity, const auto& swapchain) {
            barriers.push_back(swapchain.GenerateCurrentRenderTargetBarrier());
      });

      VkDependencyInfo window_end_barrier = {
           .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR,
           .imageMemoryBarrierCount = (uint32_t)barriers.size(),
           .pImageMemoryBarriers = barriers.data()
      };

      vkCmdPipelineBarrier2(cmd_buf, &window_end_barrier);

      if (vkEndCommandBuffer(cmd_buf) != VK_SUCCESS) {
            std::string msg = "Context::EndFrame Failed to end command buffer";
            MessageManager::Log(MessageType::Error, msg);
            throw std::runtime_error(msg);
      }

      std::vector<VkSemaphore> semaphores_wait_for{};
      std::vector<VkPipelineStageFlags> dst_stage_wait_for{};
      semaphores_wait_for.reserve(window_count);
      dst_stage_wait_for.reserve(window_count);

      _world.view<Component::Swapchain>().each([&](auto entity, auto& swapchain) {
            semaphores_wait_for.push_back(swapchain.GetCurrentSemaphore());
            dst_stage_wait_for.push_back(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
      });

      VkSubmitInfo vk_submit_info{};
      VkCommandBuffer buffers[] = { cmd_buf };
      vk_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
      vk_submit_info.commandBufferCount = 1;
      vk_submit_info.pCommandBuffers = buffers;
      vk_submit_info.pWaitSemaphores = semaphores_wait_for.data();
      vk_submit_info.waitSemaphoreCount = semaphores_wait_for.size();
      vk_submit_info.pWaitDstStageMask = dst_stage_wait_for.data();
      vk_submit_info.pSignalSemaphores = &_mainCommandQueueSemaphore[GetCurrentFrameIndex()];
      vk_submit_info.signalSemaphoreCount = 1;

      if (vkQueueSubmit(_queue, 1, &vk_submit_info, GetCurrentFence()) != VK_SUCCESS) {
            std::string msg = "Context::EndFrame Failed to submit command buffer";
            MessageManager::Log(MessageType::Error, msg);
            throw std::runtime_error(msg);
      }

      //Present
      std::vector<VkSwapchainKHR> swap_chains{};
      std::vector<uint32_t> present_images{};
      swap_chains.reserve(window_count);
      present_images.reserve(window_count);

      _world.view<Component::Swapchain>().each([&](auto entity, const Component::Swapchain& swapchain) {
            swap_chains.push_back(swapchain.GetSwapchain());
            present_images.push_back(swapchain.GetCurrentRenderTargetIndex());
      });

      VkPresentInfoKHR present_info{};
      present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
      present_info.waitSemaphoreCount = 1;
      present_info.pWaitSemaphores = &_mainCommandQueueSemaphore[GetCurrentFrameIndex()];
      present_info.pImageIndices = present_images.data();
      present_info.pSwapchains = swap_chains.data();
      present_info.swapchainCount = (uint32_t)swap_chains.size();

      auto res = vkQueuePresentKHR(_queue, &present_info);
      if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR) {
            /*for(auto& i : _windows) {
                  i.second->Update();
            }*/
      } else if (res != VK_SUCCESS) {
            std::string msg = "frame_end:Failed to present";
            MessageManager::Log(MessageType::Error, msg);
            throw std::runtime_error(msg);
      }

      GoNextFrame();
}

void Context::MakeBindlessIndexTextureForSampler(entt::entity texture, uint32_t viewIndex, std::optional<uint32_t> specifyindex) {

      if (!_world.valid(texture)) {
            std::string msg = "Context::MakeBindlessIndexTextureForSampler - Invalid texture entity";
            MessageManager::Log(MessageType::Error, msg);
            throw std::runtime_error(msg);
      }

      auto texture_component = _world.try_get<Component::Texture>(texture);

      if (!texture_component) {
            std::string msg = "Context::BindTextureForSampler - Invalid texture entity";
            MessageManager::Log(MessageType::Error, msg);
            throw std::runtime_error(msg);
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


      auto free_index = specifyindex.value_or(_bindlessIndexFreeList[2].Gen());

      VkWriteDescriptorSet write{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = _bindlessDescriptorSet,
            .dstBinding = 2,
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
}

void Context::MakeBindlessIndexTextureForComputeKernel(entt::entity texture, uint32_t viewIndex, std::optional<uint32_t> specifyindex) {

      if (!_world.valid(texture)) {
            std::string msg = "Context::MakeBindlessIndexTextureForComputeKernel - Invalid texture entity";
            MessageManager::Log(MessageType::Error, msg);
            throw std::runtime_error(msg);
      }

      auto texture_component = _world.try_get<Component::Texture>(texture);

      if (!texture_component) {
            std::string msg = "Context::BindRawTexture - Invalid texture entity";
            MessageManager::Log(MessageType::Error, msg);
            throw std::runtime_error(msg);
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

      auto free_index = specifyindex.value_or(_bindlessIndexFreeList[1].Gen());

      VkWriteDescriptorSet write{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = _bindlessDescriptorSet,
            .dstBinding = 1,
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
}

void Context::MakeBindlessIndexBuffer(entt::entity buffer, std::optional<uint32_t> specifyindex) {

      if (!_world.valid(buffer)) {
            std::string msg = "Context::BindBuffer - Invalid buffer entity";
            MessageManager::Log(MessageType::Error, msg);
            throw std::runtime_error(msg);
      }

      auto buffer_component = _world.try_get<Component::Buffer>(buffer);

      if (!buffer_component) {
            std::string msg = "Context::BindBuffer - Invalid buffer entity";
            MessageManager::Log(MessageType::Error, msg);
            throw std::runtime_error(msg);
      }

      VkDescriptorBufferInfo buffer_info = {
            .buffer = buffer_component->GetBuffer(),
            .offset = 0,
            .range = VK_WHOLE_SIZE
      };

      auto free_index = specifyindex.value_or(_bindlessIndexFreeList[0].Gen());

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
}


//Program* Context::CreateProgram(const char* source_code, const char* type, const char* entry)
//{
//      std::string_view code{ source_code };
//
//      std::vector<uint8_t> sbuffer(code.size() + 1);
//      std::ranges::copy(code, sbuffer.begin());
//      sbuffer.back() = '\0';
//
//      DxcBuffer buffer = {
//            .Ptr = sbuffer.data(),
//            .Size = (uint32_t)sbuffer.size(),
//            .Encoding = 0
//      };
//
//
//      std::vector<LPCWSTR> args{};
//      args.push_back(L"-Zpc");
//      args.push_back(L"-auto-binding-space");
//      args.push_back(L"0");
//      args.push_back(L"-HV");
//      args.push_back(L"2021");
//      args.push_back(L"-T");
//      args.push_back(L"-E");
//      args.push_back(L"main");
//      args.push_back(L"-spirv");
//      args.push_back(L"-fspv-target-env=vulkan1.3");
//      ComPtr<IDxcResult> compiled_shader{};
//      _dxcCompiler->Compile(&buffer, args.data(), (uint32_t)(args.size()), nullptr, IID_PPV_ARGS(&compiled_shader));
//      return nullptr;
//}

void Context::SetTextureSampler(entt::entity image, const VkSamplerCreateInfo& sampler_ci) {
      auto texture = _world.try_get<Component::Texture>(image);

      if (!texture) {
            std::string msg = "Context::SetTextureSampler - Invalid texture entity";
            MessageManager::Log(MessageType::Error, msg);
            throw std::runtime_error(msg);
      }

      VkSampler sampler = nullptr;

      if (auto res = _samplers.find(sampler_ci); res != _samplers.end()) {
            sampler = res->second;
            texture->SetSampler(sampler);
      } else {
            if (auto res = vkCreateSampler(_device, &sampler_ci, nullptr, &sampler); res != VK_SUCCESS) {
                  std::string msg = "Context::SetTextureSampler - Failed to create sampler";
                  MessageManager::Log(MessageType::Error, msg);
                  throw std::runtime_error(msg);
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
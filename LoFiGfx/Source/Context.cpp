//
// Created by starr on 2024/6/20.
//
#define VK_NO_PROTOTYPES
#include "Context.h"

#include "Message.h"
#include "PhysicalDevice.h"

#include <format>
#include <vector>
#include "SDL3/SDL.h"
#include "SDL3/SDL_vulkan.h"

using namespace LoFi;

Context::~Context() {
     Shutdown();
}

void Context::EnableDebug() {
      _bDebugMode = true;
}

void Context::Init() {
      volkInitialize();

      std::vector<const char*> instance_layers{ };
      if(_bDebugMode) instance_layers.push_back("VK_LAYER_KHRONOS_validation");

      std::vector needed_instance_extension {
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

            if(missing_extensions_names.size() != 0) {
                  std::string missing_extensions = "";
                  for (const auto& ext : missing_extensions_names) {
                        missing_extensions += ext;
                        missing_extensions += "\n";
                  }
                  auto error_message = std::format("Missing instance extensions: \n{}", missing_extensions);
                  MessageManager::addMessage(MessageType::Error, error_message);
                  throw std::runtime_error(error_message);
            };
      }


      VkApplicationInfo app_info{};
      app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
      app_info.pApplicationName = "LoFi";
      app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
      app_info.pEngineName = "LoFi";
      app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
      app_info.apiVersion = VK_API_VERSION_1_3;

      VkInstanceCreateInfo instance_ci {};
      instance_ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
      instance_ci.pApplicationInfo = &app_info;
      instance_ci.enabledLayerCount = instance_layers.size();
      instance_ci.ppEnabledLayerNames = instance_layers.data();
      instance_ci.enabledExtensionCount = needed_instance_extension.size();
      instance_ci.ppEnabledExtensionNames = needed_instance_extension.data();

      VkInstance instance{};
      if(vkCreateInstance(&instance_ci, nullptr, &instance) != VK_SUCCESS) {
            MessageManager::addMessage(MessageType::Error, "Failed to create Vulkan instance");
            throw std::runtime_error("Failed to create Vulkan instance");
      }
      _instance = instance;
      volkLoadInstance(instance);


      PhysicalDevice physical_device_ability_ptr{};
      {
            uint32_t count = 0;
            vkEnumeratePhysicalDevices(instance, &count, nullptr);
            std::vector<VkPhysicalDevice> physicalDevices(count);
            vkEnumeratePhysicalDevices(instance, &count, physicalDevices.data());

            for (const auto& physical_device: physicalDevices) {
                  PhysicalDevice ability(physical_device);

                  if(!ability.isQueueFamily0SupportAllQueue()) continue;
                  if(!ability._accelerationStructureFeatures.accelerationStructure) continue;
                  if(!ability._rayTracingFeatures.rayTracingPipeline) continue;
                  if(!ability._meshShaderFeatures.meshShader) continue;

                  auto finalCard = std::format("Physical device selected: {}", ability._properties2.properties.deviceName);
                  MessageManager::addMessage(MessageType::Normal, finalCard);

                  _physicalDevice = physical_device;
                  physical_device_ability_ptr = ability;
                  break;
            }

            if(!_physicalDevice) {
                  std::string error_msg = "No suitable physical device found";
                  MessageManager::addMessage(MessageType::Error, error_msg);
                  throw std::runtime_error(error_msg);
            }
      }

      {
            std::vector needed_device_extensions{
                  "VK_KHR_dynamic_rendering", // 必要的
                  "VK_EXT_descriptor_indexing", // 必要的
                  "VK_KHR_maintenance2",

                  "VK_KHR_swapchain",
                  "VK_KHR_get_memory_requirements2",
                  "VK_KHR_dedicated_allocation",
                  "VK_KHR_bind_memory2",
                  "VK_KHR_spirv_1_4",
                  "VK_KHR_synchronization2"
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

            if(missing_extensions_names.size() != 0) {
                  std::string missing_extensions = "";
                  for (const auto& ext : missing_extensions_names) {
                        missing_extensions += ext;
                        missing_extensions += "\n";
                  }
                  auto error_message = std::format("Missing device extensions: \n{}", missing_extensions);
                  MessageManager::addMessage(MessageType::Error, error_message);
                  throw std::runtime_error(error_message);
            }

            VkDeviceQueueCreateInfo queue_ci{};
            queue_ci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_ci.queueFamilyIndex = 0;
            queue_ci.queueCount = 1;
            float queue_priority = 1.0f;
            queue_ci.pQueuePriorities = &queue_priority;


            VkDeviceCreateInfo device_ci {};
            device_ci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            device_ci.pNext = &physical_device_ability_ptr._features2;
            device_ci.enabledExtensionCount = needed_device_extensions.size();
            device_ci.ppEnabledExtensionNames = needed_device_extensions.data();
            device_ci.queueCreateInfoCount = 1;
            device_ci.pQueueCreateInfos = &queue_ci;

            VkDevice device{};
            if(vkCreateDevice(_physicalDevice, &device_ci, nullptr, &device) != VK_SUCCESS) {
                  MessageManager::addMessage(MessageType::Error, "Failed to create Vulkan device");
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
	      vma_vulkan_func.vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2KHR;
	      vma_vulkan_func.vkGetDeviceBufferMemoryRequirements = vkGetDeviceBufferMemoryRequirements;
	      vma_vulkan_func.vkGetDeviceImageMemoryRequirements = vkGetDeviceImageMemoryRequirements;

	      VmaAllocatorCreateInfo vma_allocator_create_info{};
	      vma_allocator_create_info.device = _device;
	      vma_allocator_create_info.physicalDevice = _physicalDevice;
	      vma_allocator_create_info.instance = _instance;
	      vma_allocator_create_info.vulkanApiVersion = VK_API_VERSION_1_3;
	      vma_allocator_create_info.pVulkanFunctions = &vma_vulkan_func;

            if (vmaCreateAllocator(&vma_allocator_create_info, &_allocator) != VK_SUCCESS) {
                  MessageManager::addMessage(MessageType::Error, "Failed to create VMA allocator");
                  throw std::runtime_error("Failed to create VMA allocator");
            }

            volkLoadVmaAllocator(_allocator);

            vkGetDeviceQueue(_device,0, 0, &_queue);

            VkCommandPoolCreateInfo command_pool_ci{};
            command_pool_ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            command_pool_ci.queueFamilyIndex = 0;
            command_pool_ci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

            if(vkCreateCommandPool(_device, &command_pool_ci, nullptr, &_commandPool) != VK_SUCCESS) {
                  MessageManager::addMessage(MessageType::Error, "Failed to create command pool");
                  throw std::runtime_error("Failed to create command pool");
            }

            VkCommandBufferAllocateInfo command_buffer_ai{};
            command_buffer_ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            command_buffer_ai.commandPool = _commandPool;
            command_buffer_ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            command_buffer_ai.commandBufferCount = 3;

            if(vkAllocateCommandBuffers(_device, &command_buffer_ai, &_commandBuffer[0]) != VK_SUCCESS) {
                  MessageManager::addMessage(MessageType::Error, "Failed to allocate command buffers");
                  throw std::runtime_error("Failed to allocate command buffers");
            }
      }

      {
            VkFenceCreateInfo fence_ci{};
            fence_ci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fence_ci.flags = VK_FENCE_CREATE_SIGNALED_BIT;

            VkSemaphoreCreateInfo semaphore_ci{};
            semaphore_ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            for(int i = 0; i < 3; i++) {
                  vkCreateFence(_device, &fence_ci, nullptr, &_mainCommandFence[i]);
                  vkCreateSemaphore(_device, &semaphore_ci, nullptr, &_mainCommandSemaphore[i]);
            }
      }

      MessageManager::addMessage(MessageType::Normal, "Successfully initialized LoFi context");
}

void Context::Shutdown() {
      _textures.clear();
      _windows.clear();

      for(int i = 0; i < 3; i++) {
            vkDestroyFence(_device, _mainCommandFence[i], nullptr);
            vkDestroySemaphore(_device, _mainCommandSemaphore[i], nullptr);
      }
      vkFreeCommandBuffers(_device, _commandPool, 3, _commandBuffer);
      vkDestroyCommandPool(_device, _commandPool, nullptr);
      vmaDestroyAllocator(_allocator);
      vkDestroyDevice(_device, nullptr);
      vkDestroyInstance(_instance, nullptr);
}

uint32_t Context::CreateWindow(const char *title, int w, int h) {
      auto ptr = std::make_unique<Window>(title, w, h);
      auto id = ptr->GetWindowID();
      _windows.emplace(id, std::move(ptr));
      return id;
}

void Context::DestroyWindow(uint32_t id) {
      if(_windows.contains(id)) {
            _windows.erase(id);
      }
}

void * Context::PollEvent() {
      static SDL_Event event{};

      SDL_PollEvent(&event);

      if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
            const auto id = event.window.windowID;
            DestroyWindow(id);
      }

      if (event.type == SDL_EVENT_WINDOW_RESIZED) {
            const auto id = event.window.windowID;
            if(auto res = _windows.find(id); res != _windows.end() ) {
                  vkDeviceWaitIdle(_device);
                  res->second->Update();
            }
      }

      if (_windows.empty()) {
            return nullptr;
      }

      return &event;
}

Texture* Context::CreateRenderTexture(int w, int h) {
      VkImageCreateInfo image_ci{};
      image_ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
      image_ci.imageType = VK_IMAGE_TYPE_2D;
      image_ci.format = VK_FORMAT_R8G8B8A8_UNORM;
      image_ci.extent = VkExtent3D{(uint32_t)w, (uint32_t)h, 1};
      image_ci.mipLevels = 1;
      image_ci.arrayLayers = 1;
      image_ci.samples = VK_SAMPLE_COUNT_1_BIT;
      image_ci.tiling = VK_IMAGE_TILING_OPTIMAL;
      image_ci.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
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

      auto ptr = std::make_unique<Texture>(image_ci, alloc_ci);
      ptr->CreateView(view_ci);
      auto raw_ptr = ptr.get();
      _textures.insert({raw_ptr, std::move(ptr)});
      return raw_ptr;
}

Texture * Context::CreateDepthStencil(int w, int h) {

      VkImageCreateInfo image_ci{};
      image_ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
      image_ci.imageType = VK_IMAGE_TYPE_2D;
      image_ci.format = VK_FORMAT_D32_SFLOAT_S8_UINT;
      image_ci.extent = VkExtent3D{(uint32_t)w, (uint32_t)h, 1};
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

      auto ptr = std::make_unique<Texture>(image_ci, alloc_ci);
      ptr->CreateView(view_ci);
      auto raw_ptr = ptr.get();
      _textures.insert({raw_ptr, std::move(ptr)});
      return raw_ptr;
}

void Context::DestroyTexture(Texture *texture) {
      if(auto res = _textures.find(texture); res != _textures.end()){
            _textures.erase(res);
      }
}

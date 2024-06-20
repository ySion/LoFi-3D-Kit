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
            VmaAllocator vma_allocator{};
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

            _allocator = vma_allocator;

            MessageManager::addMessage(MessageType::Normal, "Successfully initialized LoFi context");
      }
}

void Context::Shutdown() {
      vmaDestroyAllocator(_allocator);
      vkDestroyDevice(_device, nullptr);
      vkDestroyInstance(_instance, nullptr);
}


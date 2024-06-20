//
// Created by starr on 2024/6/20.
//

#ifndef PHYSICALDEVICE_H
#define PHYSICALDEVICE_H

#include "Marcos.h"
#include <vector>

class PhysicalDevice {
public:
	PhysicalDevice() = default;

	explicit PhysicalDevice(VkPhysicalDevice physicalDevice);

	bool isQueueFamily0SupportAllQueue() const;

	std::vector<VkQueueFamilyProperties> _queueFamilyProperties{};

	VkPhysicalDeviceFeatures2 _features2{};

	VkPhysicalDeviceProperties2 _properties2{};

	VkPhysicalDeviceMemoryProperties2 _memoryProperties2{};

	VkPhysicalDeviceRayTracingPipelinePropertiesKHR _rayTracingPipelineProperties{};

	VkPhysicalDeviceAccelerationStructurePropertiesKHR _accelerationStructureProperties{};

	VkPhysicalDeviceRayTracingPipelineFeaturesKHR _rayTracingFeatures{};

	VkPhysicalDeviceAccelerationStructureFeaturesKHR _accelerationStructureFeatures{};

	VkPhysicalDeviceBufferDeviceAddressFeatures _bufferDeviceAddressFeatures{};

	VkPhysicalDeviceDescriptorIndexingFeatures _descriptorIndexingFeatures{};

	VkPhysicalDeviceDescriptorIndexingProperties _descriptorIndexingProperties{};

	VkPhysicalDeviceVulkan11Features _vulkan11Features{};

	VkPhysicalDeviceVulkan11Properties _vulkan11Properties{};

	VkPhysicalDeviceDynamicRenderingFeaturesKHR _dynamicRenderingFeatures{};

	VkPhysicalDeviceSynchronization2FeaturesKHR _synchronization2Features{};

	VkPhysicalDeviceMeshShaderFeaturesEXT _meshShaderFeatures{};

	VkPhysicalDeviceMeshShaderPropertiesEXT _meshShaderProperties{};

	VkPhysicalDeviceFragmentShadingRateFeaturesKHR _fragmentShadingRateFeatures{};

	VkPhysicalDeviceFragmentShadingRatePropertiesKHR _fragmentShadingRateProperties{};

};

#endif //PHYSICALDEVICE_H

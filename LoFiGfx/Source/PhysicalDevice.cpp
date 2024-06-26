//
// Created by starr on 2024/6/20.
//

#include "PhysicalDevice.h"

#include <stdexcept>

PhysicalDevice::PhysicalDevice(VkPhysicalDevice physicalDevice) {

	if(!physicalDevice) {
		throw std::runtime_error("PhysicalDevice::PhysicalDevice() - Invalid physical device");
	}

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
	_queueFamilyProperties.resize(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, _queueFamilyProperties.data());

	_features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	_features2.pNext = &_rayTracingFeatures;

	_rayTracingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
	_rayTracingFeatures.pNext = &_accelerationStructureFeatures;

	_accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
	_accelerationStructureFeatures.pNext = &_bufferDeviceAddressFeatures;

	_bufferDeviceAddressFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
	_bufferDeviceAddressFeatures.pNext = &_descriptorIndexingFeatures;

	_descriptorIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
	_descriptorIndexingFeatures.pNext = &_vulkan11Features;

	_vulkan11Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
	_vulkan11Features.pNext = &_dynamicRenderingFeatures;

	_dynamicRenderingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
	_dynamicRenderingFeatures.pNext = &_synchronization2Features;

	_synchronization2Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR;
	_synchronization2Features.pNext = &_meshShaderFeatures;

	_meshShaderFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
	_meshShaderFeatures.pNext = &_fragmentShadingRateFeatures;

	_fragmentShadingRateFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR;

	vkGetPhysicalDeviceFeatures2(physicalDevice, &_features2);

	_features2.pNext = nullptr;
	_rayTracingFeatures.pNext = nullptr;
	_accelerationStructureFeatures.pNext = nullptr;
	_bufferDeviceAddressFeatures.pNext = nullptr;
	_descriptorIndexingFeatures.pNext = nullptr;
	_vulkan11Features.pNext = nullptr;
	_dynamicRenderingFeatures.pNext = nullptr;
	_synchronization2Features.pNext = nullptr;
	_meshShaderFeatures.pNext = nullptr;
	_fragmentShadingRateFeatures.pNext = nullptr;

	//Get Properties

	_properties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	_properties2.pNext = &_rayTracingPipelineProperties;

	_rayTracingPipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
	_rayTracingPipelineProperties.pNext = &_accelerationStructureProperties;

	_accelerationStructureProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR;
	_accelerationStructureProperties.pNext = &_descriptorIndexingProperties;

	_descriptorIndexingProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_PROPERTIES;
	_descriptorIndexingProperties.pNext = &_vulkan11Properties;

	_vulkan11Properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES;
	_vulkan11Properties.pNext = &_meshShaderProperties;

	_meshShaderProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_PROPERTIES_EXT;
	_meshShaderProperties.pNext = &_fragmentShadingRateProperties;

	_fragmentShadingRateProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_PROPERTIES_KHR;

	_memoryProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;

	vkGetPhysicalDeviceProperties2(physicalDevice, &_properties2);

	vkGetPhysicalDeviceMemoryProperties2(physicalDevice, &_memoryProperties2);

}

bool PhysicalDevice::isQueueFamily0SupportAllQueue() const {

	if (_queueFamilyProperties.empty()) { return false;}

	if ((_queueFamilyProperties[0].queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
		(_queueFamilyProperties[0].queueFlags & VK_QUEUE_COMPUTE_BIT) &&
		(_queueFamilyProperties[0].queueFlags & VK_QUEUE_TRANSFER_BIT)) {
	} else {
		return false;
	}
	return true;
}

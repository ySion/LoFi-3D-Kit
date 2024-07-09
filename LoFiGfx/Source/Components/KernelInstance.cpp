#include "KernelInstance.h"

#include "Buffer.h"
#include "FrameResource.h"
#include "Kernel.h"
#include "Texture.h"

#include "../Context.h"
#include "../Message.h"

using namespace LoFi::Component;
using namespace LoFi::Internal;

KernelInstance::~KernelInstance() {
      for(const auto& i : _parameterTableBuffer.Buffers) {
            Context::Get()->DestroyHandle(i);
      }
}

KernelInstance::KernelInstance(entt::entity id, entt::entity kernel, bool high_dynamic) : _id(id), _kernel(kernel) {
      auto& world = *volkGetLoadedEcsWorld();

      if (!world.valid(id)) {
            const auto err = "KernelInstance::KernelInstance: id is invalid";
            throw std::runtime_error(err);
      }

      if (!world.valid(kernel)) {
            const auto err = "KernelInstance::KernelInstance: handle is invalid";
            throw std::runtime_error(err);
      }

      const auto* kernel_ptr = world.try_get<Kernel>(kernel);
      if (!kernel_ptr) {
            const auto err = "KernelInstance::KernelInstance: not a kernel handle";
            throw std::runtime_error(err);
      }


      const uint32_t parameter_table_size = kernel_ptr->GetParameterTableSize();

      if(kernel_ptr->GetPushConstantRange().size == 0) {
            _isEmptyInstance = true;
            return;
      }

      _resourceUsageInfo.resize(kernel_ptr->GetResourceDefine().size());

      // emplace data cache
      _parameterTableBuffer.CachedBufferData.resize(parameter_table_size * 3);
      _parameterTableSize = parameter_table_size;

      //emplace buffers
      for(int i = 0; i < 3; i++) {
            auto& buffer = _parameterTableBuffer.Buffers[i];
            buffer = Context::Get()->CreateBuffer(parameter_table_size, high_dynamic);
            _parameterTableBufferAddress[i] = world.get<Buffer>(buffer).GetAddress();
      }
}

bool KernelInstance::BindResource(const std::string& resource_name, entt::entity resource) {
      if(_isEmptyInstance) {
            const auto err = "KernelInstance::BindResource: Try to BindResource on an empty instance, ingore this operation";
            MessageManager::Log(MessageType::Warning, err);
      }
      auto& world = *volkGetLoadedEcsWorld();

      if(!world.valid(_kernel)) {
            const auto err = "KernelInstance::BindResource: parent kernel is changed or dead.";
            MessageManager::Log(MessageType::Error, err);
            return false;
      }

      if(!world.valid(resource)) {
            const auto err = "KernelInstance::BindResource: resource handle is invaild.";
            MessageManager::Log(MessageType::Error, err);
            return false;
      }

      const auto kernel_ptr = world.try_get<Kernel>(_kernel);
      if(!kernel_ptr) {
            const auto err = "KernelInstance::BindResource: parent kernel is changed or dead.";
            MessageManager::Log(MessageType::Error, err);
            return false;
      }

      const auto& resource_defines = kernel_ptr->GetResourceDefine();
      if(const auto finder = resource_defines.find(resource_name); finder == resource_defines.end()) {
            const auto err = std::format("KernelInstance::BindResource: resource {} not found in kernel.", resource_name);
            MessageManager::Log(MessageType::Error, err);
      } else {
            const uint32_t offset = finder->second.Offset;
            const uint32_t size = finder->second.Size;
            const uint32_t index = finder->second.Index;
            if((uint32_t)(finder->second.Type) <= (uint32_t)(ShaderResource::READ_WRITE_BUFFER)) { // Buffers

                  const auto ptr_buffer = world.try_get<Buffer>(resource);
                  const auto ptr_frame_resource = world.try_get<FrameResource>(resource);

                  if(!ptr_buffer && !ptr_frame_resource) {
                        const auto err = std::format("KernelInstance::BindResource: resource {} is not a Buffer or FrameResource.", resource_name);
                        MessageManager::Log(MessageType::Error, err);
                        return false;
                  }

                  if(ptr_buffer) {
                        const auto address = ptr_buffer->GetAddress();
                        memcpy(&_parameterTableBuffer.CachedBufferData.at(offset), &address, size);
                        memcpy(&_parameterTableBuffer.CachedBufferData.at(offset + _parameterTableSize), &address, size);
                        memcpy(&_parameterTableBuffer.CachedBufferData.at(offset + _parameterTableSize * 2), &address, size);
                  } else {
                        const VkDeviceAddress address0 =  ptr_frame_resource->GetBufferAddress(0);
                        const VkDeviceAddress address1 =  ptr_frame_resource->GetBufferAddress(1);
                        const VkDeviceAddress address2 =  ptr_frame_resource->GetBufferAddress(2);
                        memcpy(&_parameterTableBuffer.CachedBufferData.at(offset), &address0, size);
                        memcpy(&_parameterTableBuffer.CachedBufferData.at(offset + _parameterTableSize), &address1, size);
                        memcpy(&_parameterTableBuffer.CachedBufferData.at(offset + _parameterTableSize * 2), &address2, size);
                  }

                  switch (finder->second.Type) {
                        case ShaderResource::READ_BUFFER:
                              _resourceUsageInfo.at(index) = std::make_pair(resource, ResourceUsage::READ_BUFFER);
                        break;
                        case ShaderResource::WRITE_BUFFER:
                              _resourceUsageInfo.at(index) = std::make_pair(resource, ResourceUsage::WRITE_BUFFER);
                        break;
                        case ShaderResource::READ_WRITE_BUFFER:
                              _resourceUsageInfo.at(index) = std::make_pair(resource, ResourceUsage::READ_WRITE_BUFFER);
                        break;
                        default:
                              // unreachable
                                    break;
                  }
            } else { // Textures
                  const auto ptr_texture = world.try_get<Texture>(resource);
                  if(!ptr_texture) {
                        const auto err = std::format("KernelInstance::BindResource: resource {} is not a Texture.", resource_name);
                        MessageManager::Log(MessageType::Error, err);
                        return false;
                  }
                  if(finder->second.Type == ShaderResource::SAMPLED_TEXTURE) {
                        const auto bindless_handle = ptr_texture->GetBindlessIndexForSampler();
                        memcpy(&_parameterTableBuffer.CachedBufferData.at(offset), &bindless_handle, size);
                        memcpy(&_parameterTableBuffer.CachedBufferData.at(offset + _parameterTableSize), &bindless_handle, size);
                        memcpy(&_parameterTableBuffer.CachedBufferData.at(offset + _parameterTableSize * 2), &bindless_handle, size);
                  } else {
                        const auto bindless_handle = ptr_texture->GetBindlessIndexForSampler();
                        memcpy(&_parameterTableBuffer.CachedBufferData.at(offset), &bindless_handle, size);
                        memcpy(&_parameterTableBuffer.CachedBufferData.at(offset + _parameterTableSize), &bindless_handle, size);
                        memcpy(&_parameterTableBuffer.CachedBufferData.at(offset + _parameterTableSize * 2), &bindless_handle, size);
                  }

                  switch (finder->second.Type) {
                        case ShaderResource::SAMPLED_TEXTURE:
                              _resourceUsageInfo.at(index) = std::make_pair(resource, ResourceUsage::SAMPLED);
                        break;
                        case ShaderResource::READ_TEXTURE:
                              _resourceUsageInfo.at(index) = std::make_pair(resource, ResourceUsage::READ_TEXTURE);
                              break;
                        case ShaderResource::WRITE_TEXTURE:
                              _resourceUsageInfo.at(index) = std::make_pair(resource, ResourceUsage::WRITE_TEXTURE);
                              break;
                        case ShaderResource::READ_WRITE_TEXTURE:
                              _resourceUsageInfo.at(index) = std::make_pair(resource, ResourceUsage::READ_WRITE_TEXTURE);
                              break;
                        default:
                              // unreachable
                              break;
                  }
            }
      }

      _parameterTableBuffer.Modified = 3;
      if(!world.any_of<TagMultiFrameResourceOrBufferChanged>(_id)) {
            world.emplace<TagMultiFrameResourceOrBufferChanged>(_id);
      }

      if(world.any_of<TagMultiFrameResourceOrBufferUpdateCompleted>(_id)) {
            world.remove<TagMultiFrameResourceOrBufferUpdateCompleted>(_id);
      }

      return true;
}

void KernelInstance::PushParameterTable(VkCommandBuffer cmd) const {
      if(_isEmptyInstance) return;
      auto& world = *volkGetLoadedEcsWorld();

      if(!world.valid(_kernel)) {
            const auto err = "KernelInstance::BindResource: parent kernel is changed or dead.";
            throw std::runtime_error(err);
      }

      const auto kernel_ptr = world.try_get<Kernel>(_kernel);
      if(!kernel_ptr) {
            const auto err = "KernelInstance::BindResource: parent kernel is changed or dead.";
            throw std::runtime_error(err);
      }

      const auto current_frame = Context::Get()->GetCurrentFrameIndex();
      vkCmdPushConstants(cmd, kernel_ptr->GetPipelineLayout(), VK_SHADER_STAGE_ALL, 0, sizeof(uint64_t), &_parameterTableBufferAddress[current_frame]);
}

void KernelInstance::GenerateResourcesBarrier(VkCommandBuffer cmd) const {
      if(_isEmptyInstance) return;
      auto& world = *Internal::volkGetLoadedEcsWorld();

      if(!world.valid(_kernel)) {
            const auto err = "KernelInstance::BindResource: parent kernel is changed or dead.";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      const auto kernel_ptr = world.try_get<Kernel>(_kernel);
      if(!kernel_ptr) {
            const auto err = "KernelInstance::BindResource: parent kernel is changed or dead.";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      const KernelType kernel_type = kernel_ptr->IsComputeKernel() ? KernelType::COMPUTE : KernelType::GRAPHICS;

      for (const auto resource : _resourceUsageInfo) {
            if(resource.has_value()) {
                  switch (const auto& [resource_handle, usage] = resource.value(); usage) {
                        case ResourceUsage::READ_BUFFER:
                              if(const auto temp_buffer = world.try_get<Buffer>(resource_handle); temp_buffer) {
                                    temp_buffer->BarrierLayout(cmd, kernel_type, ResourceUsage::READ_BUFFER);
                              } else {
                                    world.get<FrameResource>(resource_handle).BarrierLayout(cmd, kernel_type, ResourceUsage::READ_BUFFER);
                              }
                              break;
                        case ResourceUsage::WRITE_BUFFER:
                              if(const auto temp_buffer = world.try_get<Buffer>(resource_handle); temp_buffer) {
                                    temp_buffer->BarrierLayout(cmd, kernel_type, ResourceUsage::WRITE_BUFFER);
                              } else {
                                    world.get<FrameResource>(resource_handle).BarrierLayout(cmd, kernel_type, ResourceUsage::WRITE_BUFFER);
                              }
                              break;
                        case ResourceUsage::READ_WRITE_BUFFER:
                              if(const auto temp_buffer = world.try_get<Buffer>(resource_handle); temp_buffer) {
                                    temp_buffer->BarrierLayout(cmd, kernel_type, ResourceUsage::READ_WRITE_BUFFER);
                              } else {
                                    world.get<FrameResource>(resource_handle).BarrierLayout(cmd, kernel_type, ResourceUsage::READ_WRITE_BUFFER);
                              }
                              break;
                        case ResourceUsage::READ_TEXTURE:
                              world.get<Texture>(resource_handle).BarrierLayout(cmd, kernel_type, ResourceUsage::READ_TEXTURE);
                              break;
                        case ResourceUsage::WRITE_TEXTURE:
                              world.get<Texture>(resource_handle).BarrierLayout(cmd, kernel_type, ResourceUsage::WRITE_TEXTURE);
                              break;
                        case ResourceUsage::READ_WRITE_TEXTURE:
                              world.get<Texture>(resource_handle).BarrierLayout(cmd, kernel_type, ResourceUsage::READ_WRITE_TEXTURE);
                              break;
                        case ResourceUsage::SAMPLED:
                              world.get<Texture>(resource_handle).BarrierLayout(cmd, kernel_type, ResourceUsage::SAMPLED);
                              break;
                        default:
                              // unreachable
                              break;
                  }
            }
      }
}

bool KernelInstance::CheckResourceSafety() const {
      for(const auto& resource : _resourceUsageInfo) {
            if(!resource.has_value()) {
                  return false;
            }
      }
      return true;
}

void KernelInstance::UpdateAll() {
      auto& world = *Internal::volkGetLoadedEcsWorld();
      world.view<KernelInstance, TagMultiFrameResourceOrBufferChanged>().each([](auto entity, Component::KernelInstance& instance) {
            instance.UpdateParameterTable();
      });

      const auto p = world.view<KernelInstance, TagMultiFrameResourceOrBufferChanged, TagMultiFrameResourceOrBufferUpdateCompleted>();
      world.remove<TagMultiFrameResourceOrBufferChanged, TagMultiFrameResourceOrBufferUpdateCompleted>(p.begin(), p.end());
}

void KernelInstance::UpdateParameterTable() {
      if(_isEmptyInstance) return;
      if(_parameterTableBuffer.Modified == 0) return;

      auto& world = *volkGetLoadedEcsWorld();

      const auto current_frame = Context::Get()->GetCurrentFrameIndex();
      const auto buffer = _parameterTableBuffer.Buffers[current_frame];

      auto& buffer_ptr = world.get<Buffer>(buffer);
      buffer_ptr.SetData(&_parameterTableBuffer.CachedBufferData.at(_parameterTableSize * current_frame), _parameterTableSize);

      _parameterTableBuffer.Modified--;
      if(_parameterTableBuffer.Modified == 0) {
            world.emplace<TagMultiFrameResourceOrBufferUpdateCompleted>(_id);
      }
}

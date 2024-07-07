#include "KernelInstance.h"

#include "Buffer.h"
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

      // emplace data cache
      _parameterTableBuffer.CachedBufferData.resize(parameter_table_size);

      //emplace buffers

      for(int i = 0; i < 3; i++) {
            auto& buffer = _parameterTableBuffer.Buffers[i];
            buffer = Context::Get()->CreateBuffer(parameter_table_size, high_dynamic);
            _parameterTableBufferAddress[i] = world.get<Buffer>(buffer).GetAddress();
      }
}

bool KernelInstance::BindResource(const std::string& resource_name, entt::entity resource) {
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

            if((uint32_t)(finder->second.Type) <= (uint32_t)(ShaderResource::READ_WRITE_BUFFER)) { // Buffers
                  const auto ptr_buffer = world.try_get<Buffer>(resource);
                  if(!ptr_buffer) {
                        const auto err = std::format("KernelInstance::BindResource: resource {} is not a buffer.", resource_name);
                        MessageManager::Log(MessageType::Error, err);
                        return false;
                  }

                  const auto address = ptr_buffer->GetAddress();
                  memcpy(&_parameterTableBuffer.CachedBufferData.at(offset), &address, size);
            } else { // Textures
                  const auto ptr_texture = world.try_get<Texture>(resource);
                  if(!ptr_texture) {
                        const auto err = std::format("KernelInstance::BindResource: resource {} is not a texture.", resource_name);
                        MessageManager::Log(MessageType::Error, err);
                        return false;
                  }

                  if(finder->second.Type == ShaderResource::SAMPLED_TEXTURE) {
                        const auto bindless_handle = ptr_texture->GetBindlessIndexForSampler();
                        memcpy(&_parameterTableBuffer.CachedBufferData.at(offset), &bindless_handle, size);
                  } else {
                        const auto bindless_handle = ptr_texture->GetBindlessIndexForSampler();
                        memcpy(&_parameterTableBuffer.CachedBufferData.at(offset), &bindless_handle, size);
                  }
            }
      }

      _parameterTableBuffer.Modified = 3;
      if(!world.any_of<TagKernelInstanceParamChanged>(_id)) {
            world.emplace<TagKernelInstanceParamChanged>(_id);
      }

      if(world.any_of<TagKernelInstanceParamUpdateCompleted>(_id)) {
            world.remove<TagKernelInstanceParamUpdateCompleted>(_id);
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

void KernelInstance::UpdateInstancesParameterTable() {
      auto& world = *Internal::volkGetLoadedEcsWorld();
      world.view<KernelInstance, TagKernelInstanceParamChanged>().each([](auto entity, Component::KernelInstance& instance) {
            instance.UpdateParameterTable();
      });

      const auto p = world.view<KernelInstance, TagKernelInstanceParamChanged, TagKernelInstanceParamUpdateCompleted>();
      world.remove<TagKernelInstanceParamChanged, TagKernelInstanceParamUpdateCompleted>(p.begin(), p.end());
}

void KernelInstance::UpdateParameterTable() {
      if(_isEmptyInstance) return;
      if(_parameterTableBuffer.Modified == 0) return;

      auto& world = *volkGetLoadedEcsWorld();

      const auto current_frame = Context::Get()->GetCurrentFrameIndex();
      const auto buffer = _parameterTableBuffer.Buffers[current_frame];

      auto& buffer_ptr = world.get<Buffer>(buffer);
      buffer_ptr.SetData(_parameterTableBuffer.CachedBufferData.data(), _parameterTableBuffer.CachedBufferData.size());

      _parameterTableBuffer.Modified--;
      if(_parameterTableBuffer.Modified == 0) {
            world.emplace<TagKernelInstanceParamUpdateCompleted>(_id);
      }
}

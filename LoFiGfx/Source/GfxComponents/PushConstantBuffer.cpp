#include "PushConstantBuffer.h"

#include "Kernel.h"
#include "Program.h"

#include "../Message.h"

using namespace LoFi::Component::Gfx;
using namespace LoFi::Internal;


PushConstantBuffer::~PushConstantBuffer() = default;

PushConstantBuffer::PushConstantBuffer(entt::entity id, entt::entity kernel) : _id(id), _kernel(kernel){
      auto& world = *volkGetLoadedEcsWorld();

      if(!world.valid(id)) {
            const auto err = "PushConstantBuffer::PushConstantBuffer: id is invalid";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      if(!world.valid(kernel)) {
            const auto err = "PushConstantBuffer::PushConstantBuffer: handle is invalid";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      const auto kernel_ptr = world.try_get<Kernel>(kernel);
      if(!kernel_ptr) {
            const auto err = "PushConstantBuffer::PushConstantBuffer: not a kernel handle";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      const auto push_constant_range = kernel_ptr->GetPushConstantRange();
      _pushConstantBuffer.resize(push_constant_range.size);

      _pushConstantDefine = &kernel_ptr->GetPushConstantDefine();
}

void PushConstantBuffer::SetValue(const std::string& name, const void* data) {
      auto& world = *volkGetLoadedEcsWorld();

      if(!world.valid(_kernel)) {
            const auto err = "PushConstantBuffer::SetValue: parent kernel has changed or dead";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      if(const auto parameter_find = _pushConstantDefine->find(name); parameter_find != _pushConstantDefine->end()) {
            const auto& [Offset, Size] = parameter_find->second;
            std::memcpy(&_pushConstantBuffer.at(Offset), data, Size);
      } else {
            const auto err = "PushConstantBuffer::SetValue: parameter not found";
            MessageManager::Log(MessageType::Warning, err);
      }
}

void PushConstantBuffer::CmdPushConstants(VkCommandBuffer cmd) const {
      auto& world = *volkGetLoadedEcsWorld();
      if(!world.valid(_kernel)) { return; }
      vkCmdPushConstants(cmd, _pipelineLayout, VK_SHADER_STAGE_ALL, 0, _pushConstantBuffer.size(), _pushConstantBuffer.data());
}

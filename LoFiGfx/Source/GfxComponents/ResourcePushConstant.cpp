#include "ResourcePushConstant.h"

#include "Kernel.h"
#include "Program.h"

#include "../Message.h"

using namespace LoFi::Component::Gfx;
using namespace LoFi::Internal;


ResourcePushConstant::~ResourcePushConstant() {

}

ResourcePushConstant::ResourcePushConstant(entt::entity id, entt::entity kernel) : _id(id), _kernel(kernel){
      auto& world = *volkGetLoadedEcsWorld();

      if(!world.valid(id)) {
            const auto err = "ResourcePushConstant::ResourcePushConstant: id is invalid";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      if(!world.valid(kernel)) {
            const auto err = "ResourcePushConstant::ResourcePushConstant: handle is invalid";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      const auto kernel_ptr = world.try_get<Kernel>(kernel);
      if(!kernel_ptr) {
            const auto err = "ResourcePushConstant::ResourcePushConstant: not a kernel handle";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      const auto push_constant_range = kernel_ptr->GetPushConstantRange();
      _pushConstantBuffer.resize(push_constant_range.size);
}

void ResourcePushConstant::SetValue(const std::string& name, const void* data) {
      auto& world = *volkGetLoadedEcsWorld();

      if(!world.valid(_kernel)) {
            const auto err = "ResourcePushConstant::SetValue: parent kernel has changed or dead";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      const auto kernel_ptr = world.try_get<Kernel>(_kernel);
      const auto& push_constant_define = kernel_ptr->GetPushConstantDefine();

      if(const auto parameter_find = push_constant_define.find(name); parameter_find != push_constant_define.end()) {
            const auto& [Offset, Size] = parameter_find->second;
            std::memcpy(&_pushConstantBuffer.at(Offset), data, Size);
      } else {
            const auto err = "ResourcePushConstant::SetValue: parameter not found";
            MessageManager::Log(MessageType::Warning, err);
      }
}

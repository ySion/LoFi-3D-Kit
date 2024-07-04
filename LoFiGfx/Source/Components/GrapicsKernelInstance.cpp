//
// Created by Arzuo on 2024/6/28.
//

#include "GrapicsKernelInstance.h"
#include "../Context.h"
#include "../Message.h"
using namespace LoFi::Component;
using namespace LoFi::Internal;

GrapicsKernelInstance::~GrapicsKernelInstance() {
      auto& world = *volkGetLoadedEcsWorld();
      for (const auto& resourece_buffer : _paramBuffers) {
            for (int i = 0; i < 3; i++) {
                  world.destroy(resourece_buffer.Buffers[i]);
            }
      }
}

GrapicsKernelInstance::GrapicsKernelInstance(entt::entity id, entt::entity graphics_kernel, bool param_high_dynamic) : _id(id), _isParamHighDynamic(param_high_dynamic) {
      auto& world = *volkGetLoadedEcsWorld();

      if(!world.valid(id)) {
            const auto err = std::format("GrapicsKernelInstance::GrapicsKernelInstance - Invalid Entity ID\n");
            MessageManager::Log(MessageType::Warning, err);
            return;
      }

      auto& ctx = *Context::Get();

      if (!world.valid(graphics_kernel)) {
            const auto err = std::format("GrapicsKernelInstance::GrapicsKernelInstance - Invalid Graphics Kernel Entity\n");
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      auto kernel = world.try_get<GraphicKernel>(graphics_kernel);
      if (!kernel) {
            const auto err = std::format("GrapicsKernelInstance::GrapicsKernelInstance - this entity is not a Graphics Kernel.\n");
            MessageManager::Log(MessageType::Warning, err);
            throw std::runtime_error(err);
      }

      const auto& arg_count = kernel->GetReourceDefineTable().size();
      _pushConstantBindlessIndexInfoBuffer.resize(arg_count); //resize push constant buffer
      _resourceBind.resize(arg_count); // RESOURCE bind

      const auto& struct_table = kernel->GetParamTable();
      _paramBuffers.resize(struct_table.size()); // PARAM buffers

      for (const auto& i : struct_table) {

            KernelParamResource& buffer = _paramBuffers.at(i.second.Index);
            const auto buffer_index = i.second.Index;
            buffer.CachedBufferData.resize(i.second.Size);
           
            for (int idx = 0; idx < 3; idx++) {

                  auto& it = buffer.Buffers[idx];
                  const auto buffer_created = ctx.CreateBuffer(buffer.CachedBufferData.size(), param_high_dynamic);

                  if(!world.valid(buffer_created)) { //if happend, all dead
                        const auto err = std::format("GrapicsKernelInstance::GrapicsKernelInstance - Can't Allocate buffer! crashed\n");
                        MessageManager::Log(MessageType::Error, err);
                        throw std::runtime_error(err);
                  }

                  buffer.Buffers[idx] = buffer_created;

                  const auto& buffer_comp = world.get<Buffer>(buffer_created);
                  const auto buffer_bindless_index = buffer_comp.GetBindlessIndex();

                  if(!buffer_bindless_index.has_value()) { //should not happend
                        const auto err = std::format("GrapicsKernelInstance::GrapicsKernelInstance - Buffer has no bindless index ? it's should not happened\n");
                        MessageManager::Log(MessageType::Error, err);
                        throw std::runtime_error(err);
                  }

                  //printf("\tGrapicsKernelInstance Struct: %s - index: %u, size: %u.\n", i.first.c_str(), buffer_index, i.second.Size);
                  //printf("\t\t create at offset %u in push constant buffer[%u].\n", buffer_index, idx);

                  _pushConstantBindlessIndexInfoBuffer.at(buffer_index) = buffer_bindless_index.value();
            }
      }
      _parent = graphics_kernel;
}

bool GrapicsKernelInstance::SetParam(const std::string& param_struct_name, const void* data) {
      auto& world = *volkGetLoadedEcsWorld();

      if (!world.valid(_parent)) {
            const auto err = std::format("GrapicsKernelInstance::SetParam - Invalid Parent Entity\n");
            MessageManager::Log(MessageType::Error, err);
            return false;
      }

      if (const auto parent_kernel = world.try_get<GraphicKernel>(_parent); parent_kernel) {
            const auto& map = parent_kernel->GetParamTable();
            if (const auto finder = map.find(param_struct_name); finder != map.end()) {
                  const auto& [index, size] = finder->second;

                  uint8_t* ptr = _paramBuffers.at(index).CachedBufferData.data();
                  std::memcpy(ptr, data, size);
                  _paramBuffers.at(index).Modified = 3;
            } else {
                  const auto err = std::format("GrapicsKernelInstance::SetParam - PARAM \"{}\" Not Found\n", param_struct_name);
                  MessageManager::Log(MessageType::Error, err);
                  return false;
            }
      } else if (const auto parent_framebuffer = world.try_get<GrapicsKernelInstance>(_parent); parent_framebuffer) {
            // TODO

            return false;
      } else {
            const auto err = std::format("GrapicsKernelInstance::SetParam - Parent Entity is not a Graphics Kernel or GrapicsKernelInstance\n");
            MessageManager::Log(MessageType::Error, err);
            return false;
      }

      if(!world.any_of<TagKernelInstanceParamChanged>(_id)) {
            world.emplace<TagKernelInstanceParamChanged>(_id);
      }

      return true;
}

bool GrapicsKernelInstance::SetParamMember(const std::string& param_struct_member_name, const void* data) {
      auto& world = *volkGetLoadedEcsWorld();

      if (!world.valid(_parent)) {
            const auto err = std::format("GrapicsKernelInstance::SetParamMember - Invalid Parent Entity\n");
            MessageManager::Log(MessageType::Error, err);
            return false;
      }

      if (const auto parent_kernel = world.try_get<GraphicKernel>(_parent); parent_kernel) {
            const auto& map = parent_kernel->GetParamMemberTable();

            if (const auto finder = map.find(param_struct_member_name); finder != map.end()) {
                  auto [index, size, offset] = finder->second;

                  uint8_t* struct_start_ptr = _paramBuffers.at(index).CachedBufferData.data();
                  memcpy(struct_start_ptr + offset, data, size);
                  _paramBuffers.at(index).Modified = 3;
            } else {
                  const auto err = std::format("GrapicsKernelInstance::SetParamMember - Struct Member \"{}\" Not Found", param_struct_member_name);
                  MessageManager::Log(MessageType::Error, err);
                  return false;
            }
      } else if (const auto parent_framebuffer = world.try_get<GrapicsKernelInstance>(_parent); parent_framebuffer) {
            // TODO

            return false;
      } else {
            const auto err = std::format("GrapicsKernelInstance::SetParamMember - Parent Entity is not a Graphics Kernel or GrapicsKernelInstance\n");
            MessageManager::Log(MessageType::Error, err);
            return false;
      }

      if(!world.any_of<TagKernelInstanceParamChanged>(_id)) {
            world.emplace<TagKernelInstanceParamChanged>(_id);
      }

      return true;
}

bool GrapicsKernelInstance::SetSampled(const std::string& sampled_name, entt::entity texture) {
      auto& world = *volkGetLoadedEcsWorld();

      if(!world.valid(texture)) {
            const auto err = std::format("GrapicsKernelInstance::SetSampled - Invalid Texture Entity\n");
            MessageManager::Log(MessageType::Error, err);
            return false;
      }

      auto texture_comp = world.try_get<Texture>(texture);
      if(!texture_comp) {
            const auto err = std::format("GrapicsKernelInstance::SetSampled - This entity is not a texture \n");
            MessageManager::Log(MessageType::Error, err);
            return false;
      }

      auto bindless_index = texture_comp->GetBindlessIndexForSampler();
      if(!bindless_index.has_value()) {
            const auto err = std::format("GrapicsKernelInstance::SetSampled - Texture has no bindless index\n");
            MessageManager::Log(MessageType::Error, err);
            return false;
      }

      if (!world.valid(_parent)) {
            const auto err = std::format("GrapicsKernelInstance::SetSampled - Invalid Parent Entity\n");
            MessageManager::Log(MessageType::Error, err);
            return false;
      }

      if (const auto parent_kernel = world.try_get<GraphicKernel>(_parent); parent_kernel) {
            const auto& map = parent_kernel->GetReourceDefineTable();
            if (const auto finder = map.find(sampled_name); finder != map.end()) {
                  if(finder->second.Type == ProgramShaderReourceType::SAMPLED) {
                        _resourceBind.at(finder->second.Index) = std::make_pair(texture, finder->second.Type);
                        _pushConstantBindlessIndexInfoBuffer.at(finder->second.Index) = bindless_index.value();
                  } else {
                        const auto err = std::format("GrapicsKernelInstance::SetSampled - Resource \"{}\" is not a sampled resource\n", sampled_name);
                        MessageManager::Log(MessageType::Error, err);
                        return false;
                  }
            } else {
                  const auto err = std::format("GrapicsKernelInstance::SetSampled - Texture \"{}\" Not Found\n", sampled_name);
                  MessageManager::Log(MessageType::Error, err);
                  return false;
            }
      } else {
            const auto err = std::format("GrapicsKernelInstance::SetSampled - Parent Entity is not a Graphics Kernel\n");
            MessageManager::Log(MessageType::Error, err);
            return false;
      }

      return true;
}

void GrapicsKernelInstance::PushResourceChanged() {

      auto& world = *volkGetLoadedEcsWorld();
      if(!world.any_of<TagKernelInstanceParamChanged>(_id))  return;

      const auto current_frame = Context::Get()->GetCurrentFrameIndex();
      for (auto& buffer : _paramBuffers) {
            if (buffer.Modified > 0) {
                  const auto buf = buffer.Buffers[current_frame];
                  world.get<Buffer>(buf).SetData(buffer.CachedBufferData.data(), buffer.CachedBufferData.size());
                  buffer.Modified--;
                  if(buffer.Modified == 0) {
                        world.emplace<TagKernelInstanceParamUpdateCompleted>(_id);
                  }
            }
      }
}

void GrapicsKernelInstance::PushBindlessInfo(VkCommandBuffer buf) const {
      auto& world = *volkGetLoadedEcsWorld();
      if (!world.valid(_parent)) {
            const auto err = std::format("GrapicsKernelInstance::PushBindlessInfo - Invalid Parent Graphics Kernel Entity!\n");
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      if (const auto parent_kernel = world.try_get<GraphicKernel>(_parent); parent_kernel) {
            const auto& push_constant_range = parent_kernel->GetBindlessInfoPushConstantRange();
            vkCmdPushConstants(buf, parent_kernel->GetPipelineLayout(), VK_SHADER_STAGE_ALL, push_constant_range.offset, push_constant_range.size, _pushConstantBindlessIndexInfoBuffer.data());
      } else {
            const auto err = std::format("GrapicsKernelInstance::PushBindlessInfo - Parent Entity is not a Graphics Kernel\n");
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }
}


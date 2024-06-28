//
// Created by Arzuo on 2024/6/28.
//

#include "FrameResource.h"
#include "../Context.h"
#include "../Message.h"
using namespace LoFi::Component;
using namespace LoFi::Internal;

FrameResource::~FrameResource() {
      auto& world = *volkGetLoadedEcsWorld();
      for (const auto& resourece_buffer : _buffers) {
            for (int i = 0; i < 3; i++) {
                  world.destroy(resourece_buffer.Buffers[i]);
            }
      }
}

FrameResource::FrameResource(entt::entity id, entt::entity graphics_kernel, bool is_cpu_side) : _id(id), _isCpuSide(is_cpu_side) {

      auto& ctx = *Context::Get();
      auto& world = *volkGetLoadedEcsWorld();

      if (!world.valid(graphics_kernel)) {
            const auto err = std::format("FrameResource::FrameResource - Invalid Graphics Kernel Entity\n");
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      auto kernel = world.try_get<GraphicKernel>(graphics_kernel);
      if (!kernel) {
            const auto err = std::format("FrameResource::FrameResource - this entity is not a Graphics Kernel.\n");
            MessageManager::Log(MessageType::Warning, err);
            throw std::runtime_error(err);
      }

      const auto& struct_table = kernel->GetStructTable();
      for (int idx = 0; idx < 3; idx++) {
          _pushConstantBindlessIndexInfoBuffer.at(idx).resize(struct_table.size());
      }

      _buffers.resize(struct_table.size());

      for (const auto& i : struct_table) {

            FrameResourceBuffer& buffer = _buffers.at(i.second.Index);
            const auto buffer_index = i.second.Index;
            buffer.CachedBufferData.resize(i.second.Size);
           
            for (int idx = 0; idx < 3; idx++) {

                  auto& it = buffer.Buffers[idx];
                  const auto buffer_created = ctx.CreateBuffer(buffer.CachedBufferData.size(), is_cpu_side);

                  if(!world.valid(buffer_created)) { //if happend, all dead
                        const auto err = std::format("FrameResource::FrameResource - Can't Allocate buffer! crashed\n");
                        MessageManager::Log(MessageType::Error, err);
                        throw std::runtime_error(err);
                  }

                  buffer.Buffers[idx] = buffer_created;

                  const auto& buffer_comp = world.get<Buffer>(buffer_created);
                  const auto buffer_bindless_index = buffer_comp.GetBindlessIndex();

                  if(!buffer_bindless_index.has_value()) { //should not happend
                        const auto err = std::format("FrameResource::FrameResource - Buffer has no bindless index ? it's should not happened\n");
                        MessageManager::Log(MessageType::Error, err);
                        throw std::runtime_error(err);
                  }

                  //printf("\tFrameResource Struct: %s - index: %u, size: %u.\n", i.first.c_str(), buffer_index, i.second.Size);
                  //printf("\t\t create at offset %u in push constant buffer[%u].\n", buffer_index, idx);

                  _pushConstantBindlessIndexInfoBuffer.at(idx).at(buffer_index) = buffer_bindless_index.value();
            }
      }
      _parent = graphics_kernel;
      printf("Create Resource Success! %u", _parent);
}

bool FrameResource::SetStruct(const std::string& struct_name, const void* data) {
      auto& world = *volkGetLoadedEcsWorld();

      if (!world.valid(_parent)) {
            const auto err = std::format("FrameResource::SetStruct - Invalid Parent Entity\n");
            MessageManager::Log(MessageType::Error, err);
            return false;
      }

      if (const auto parent_kernel = world.try_get<GraphicKernel>(_parent); parent_kernel) {
            const auto& map = parent_kernel->GetStructTable();
            if (const auto finder = map.find(struct_name); finder != map.end()) {
                  GraphicKernelStructInfo info = finder->second;
                  const auto index = info.Index;
                  const auto size = info.Size;

                  uint8_t* ptr = _buffers.at(index).CachedBufferData.data();
                  std::memcpy(ptr, data, size);
                  _buffers.at(index).IsModified = true;
            } else {
                  const auto err = std::format("FrameResource::SetStruct - Struct \"{}\" Not Found\n", struct_name);
                  MessageManager::Log(MessageType::Error, err);
                  return false;
            }
      } else if (const auto parent_framebuffer = world.try_get<FrameResource>(_parent); parent_framebuffer) {
            // TODO

            return false;
      } else {
            const auto err = std::format("FrameResource::SetStruct - Parent Entity is not a Graphics Kernel or FrameResource\n");
            MessageManager::Log(MessageType::Error, err);
            return false;
      }

      if(!world.any_of<TagFrameResourceChanged>(_id)) {
            world.emplace<TagFrameResourceChanged>(_id);
      }

      return true;
}

bool FrameResource::SetStructMember(const std::string& struct_member_name, const void* data) {
      auto& world = *volkGetLoadedEcsWorld();

      if (!world.valid(_parent)) {
            const auto err = std::format("FrameResource::SetStructMember - Invalid Parent Entity\n");
            MessageManager::Log(MessageType::Error, err);
            return false;
      }

      if (const auto parent_kernel = world.try_get<GraphicKernel>(_parent); parent_kernel) {
            const auto& map = parent_kernel->GetStructMemberTable();

            if (const auto finder = map.find(struct_member_name); finder != map.end()) {
                  GraphicKernelStructMemberInfo info = finder->second;
                  const auto index = info.StructIndex;
                  const auto size = info.Size;
                  const auto offset = info.Offset;

                  uint8_t* struct_start_ptr = _buffers.at(index).CachedBufferData.data();
                  memcpy(struct_start_ptr + offset, data, size);
                  _buffers.at(index).IsModified = true;
            } else {
                  const auto err = std::format("FrameResource::SetStructMember - Struct Member \"{}\" Not Found\n", struct_member_name);
                  MessageManager::Log(MessageType::Error, err);
                  return false;
            }
      } else if (const auto parent_framebuffer = world.try_get<FrameResource>(_parent); parent_framebuffer) {
            // TODO

            return false;
      } else {
            const auto err = std::format("FrameResource::SetStructMember - Parent Entity is not a Graphics Kernel or FrameResource\n");
            MessageManager::Log(MessageType::Error, err);
            return false;
      }

      if(!world.any_of<TagFrameResourceChanged>(_id)) {
            world.emplace<TagFrameResourceChanged>(_id);
      }

      return true;
}

bool FrameResource::SetSampledImage(const std::string& image_name, entt::entity texture) {
      return false;
}

void FrameResource::PushResourceChanged() {

      auto& world = *volkGetLoadedEcsWorld();
      if(!world.any_of<TagFrameResourceChanged>(_id))  return;

      const auto current_frame = Context::Get()->GetCurrentFrameIndex();
      for (auto& buffer : _buffers) {
            if (buffer.IsModified) {
                  const auto buf = buffer.Buffers[current_frame];
                  world.get<Buffer>(buf).SetData(buffer.CachedBufferData.data(), buffer.CachedBufferData.size());
                  buffer.IsModified = false;
            }
      }
}

void FrameResource::PushBindlessInfo(VkCommandBuffer buf) const {
      auto& world = *volkGetLoadedEcsWorld();
      if (!world.valid(_parent)) {
            const auto err = std::format("FrameResource::SetStructMember - Invalid Parent Graphics Kernel Entity!\n");
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      if (const auto parent_kernel = world.try_get<GraphicKernel>(_parent); parent_kernel) {
            const auto& push_constant_range = parent_kernel->GetBindlessInfoPushConstantRange();
            const auto current_frame = Context::Get()->GetCurrentFrameIndex();
            vkCmdPushConstants(buf, parent_kernel->GetPipelineLayout(), VK_SHADER_STAGE_ALL, push_constant_range.offset, push_constant_range.size, _pushConstantBindlessIndexInfoBuffer[current_frame].data());
      } else {
            const auto err = std::format("FrameResource::SetStructMember - Parent Entity is not a Graphics Kernel\n");
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }
}


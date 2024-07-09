#include "FrameResource.h"
#include "Buffer.h"

#include "../Context.h"

using namespace LoFi::Component;
using namespace LoFi::Internal;

FrameResource::~FrameResource() {
      for(auto& buffer : _buffers) {
            Context::Get()->DestroyHandle(buffer);
      }
}

FrameResource::FrameResource(entt::entity id, uint64_t size, bool high_dynamic) : _id(id) {
      auto& world = *volkGetLoadedEcsWorld();

      if (!world.valid(id)) {
            const auto err = "FrameResource::FrameResource: id is invalid";
            throw std::runtime_error(err);
      }

      _dataCache.resize(size);

      _buffers = {
            Context::Get()->CreateBuffer(size, high_dynamic),
            Context::Get()->CreateBuffer(size, high_dynamic),
            Context::Get()->CreateBuffer(size, high_dynamic)
      };
}

entt::entity FrameResource::GetBuffer() const {
      uint32_t current_frame_index = Context::Get()->GetCurrentFrameIndex();
      return GetBuffer(current_frame_index);
}

VkDeviceAddress FrameResource::GetBufferAddress(uint32_t idx) const {
      auto& world = *volkGetLoadedEcsWorld();
      auto& buffer = world.get<Buffer>(_buffers.at(idx));
      return buffer.GetAddress();
}

void FrameResource::BarrierLayout(VkCommandBuffer cmd, KernelType new_kernel_type, ResourceUsage new_usage) const {
      auto& world = *volkGetLoadedEcsWorld();
      uint32_t current_frame_index = Context::Get()->GetCurrentFrameIndex();
      world.get<Buffer>(_buffers.at(current_frame_index)).BarrierLayout(cmd, new_kernel_type, new_usage);
}

void FrameResource::SetData(void* p, uint64_t size, uint64_t offset) {

      const uint64_t copy_size =  _dataCache.size() - offset < size ? _dataCache.size() - offset : size;
      memcpy(&_dataCache.at(offset), p, copy_size);

      _dirty = 3;

      auto& world = *volkGetLoadedEcsWorld();

      if(!world.any_of<TagMultiFrameResourceOrBufferChanged>(_id)) {
            world.emplace<TagMultiFrameResourceOrBufferChanged>(_id);
      }

      if(world.any_of<TagMultiFrameResourceOrBufferUpdateCompleted>(_id)) {
            world.remove<TagMultiFrameResourceOrBufferUpdateCompleted>(_id);
      }
}

void FrameResource::UpdateAll() {
      auto& world = *Internal::volkGetLoadedEcsWorld();
      world.view<FrameResource, TagMultiFrameResourceOrBufferChanged>().each([](auto entity, FrameResource& instance) {
            instance.UpdateFrameResource();
      });

      const auto p = world.view<FrameResource, TagMultiFrameResourceOrBufferChanged, TagMultiFrameResourceOrBufferUpdateCompleted>();
      world.remove<TagMultiFrameResourceOrBufferChanged, TagMultiFrameResourceOrBufferUpdateCompleted>(p.begin(), p.end());
}

void FrameResource::UpdateFrameResource() {
      if(_dirty == 0) return;

      const auto frame_index = Context::Get()->GetCurrentFrameIndex();

      _dirty--;

      auto& world = *volkGetLoadedEcsWorld();

      auto& buffer = world.get<Buffer>(_buffers[frame_index]);
      buffer.SetData(_dataCache.data(), _dataCache.size());

      if(_dirty == 0) {
            world.emplace<TagMultiFrameResourceOrBufferUpdateCompleted>(_id);
      }
}

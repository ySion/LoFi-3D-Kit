#include "Buffer3F.h"
#include "Buffer.h"

#include "../GfxContext.h"

using namespace LoFi::Component::Gfx;
using namespace LoFi::Internal;

Buffer3F::~Buffer3F() {
      for(auto& buffer : _buffers) {
            GfxContext::Get()->DestroyHandle(buffer);
      }
}

Buffer3F::Buffer3F(entt::entity id, uint64_t size, bool high_dynamic) : _id(id) {
      auto& world = *volkGetLoadedEcsWorld();

      if (!world.valid(id)) {
            const auto err = "FrameResource::FrameResource: id is invalid";
            throw std::runtime_error(err);
      }

      _dataCache.resize(size);

      _buffers = {
            GfxContext::Get()->CreateBuffer(size, high_dynamic),
            GfxContext::Get()->CreateBuffer(size, high_dynamic),
            GfxContext::Get()->CreateBuffer(size, high_dynamic)
      };
}

entt::entity Buffer3F::GetBuffer() const {
      uint32_t current_frame_index = GfxContext::Get()->GetCurrentFrameIndex();
      return GetBuffer(current_frame_index);
}

VkDeviceAddress Buffer3F::GetBufferAddress(uint32_t idx) const {
      auto& world = *volkGetLoadedEcsWorld();
      auto& buffer = world.get<Buffer>(_buffers.at(idx));
      return buffer.GetAddress();
}

void Buffer3F::BarrierLayout(VkCommandBuffer cmd, KernelType new_kernel_type, ResourceUsage new_usage) const {
      auto& world = *volkGetLoadedEcsWorld();
      uint32_t current_frame_index = GfxContext::Get()->GetCurrentFrameIndex();
      world.get<Buffer>(_buffers.at(current_frame_index)).BarrierLayout(cmd, new_kernel_type, new_usage);
}

void Buffer3F::SetData(void* p, uint64_t size, uint64_t offset) {

      const uint64_t copy_size =  _dataCache.size() - offset < size ? _dataCache.size() - offset : size;
      memcpy(&_dataCache.at(offset), p, copy_size);

      _dirty = 3;

      auto& world = *volkGetLoadedEcsWorld();

      if(!world.any_of<TagDataChanged>(_id)) {
            world.emplace<TagDataChanged>(_id);
      }

      if(world.any_of<TagDataUpdateCompleted>(_id)) {
            world.remove<TagDataUpdateCompleted>(_id);
      }
}

void Buffer3F::UpdateAll() {
      auto& world = *Internal::volkGetLoadedEcsWorld();
      world.view<Buffer3F, TagDataChanged>().each([](auto entity, Buffer3F& instance) {
            instance.UpdateFrameResource();
      });

      const auto p = world.view<Buffer3F, TagDataChanged, TagDataUpdateCompleted>();
      world.remove<TagDataChanged, TagDataUpdateCompleted>(p.begin(), p.end());
}

void Buffer3F::UpdateFrameResource() {
      if(_dirty == 0) return;

      const auto frame_index = GfxContext::Get()->GetCurrentFrameIndex();

      _dirty--;

      auto& world = *volkGetLoadedEcsWorld();

      auto& buffer = world.get<Buffer>(_buffers[frame_index]);
      buffer.SetData(_dataCache.data(), _dataCache.size());

      if(_dirty == 0) {
            world.emplace<TagDataUpdateCompleted>(_id);
      }
}

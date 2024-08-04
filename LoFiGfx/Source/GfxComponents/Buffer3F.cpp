#include "Buffer3F.h"
#include "Buffer.h"

#include "../GfxContext.h"
#include "../Message.h"

using namespace LoFi::Component::Gfx;
using namespace LoFi::Internal;

Buffer3F::Buffer3F(entt::entity id) : _id(id) {}

Buffer3F::~Buffer3F() {
      for(auto& buffer : _buffers) {
            buffer.reset();
      }
}

bool Buffer3F::Init(const GfxParamCreateBuffer3F& param) {
      _resourceName = param.pResourceName ? param.pResourceName : std::string{};
      _dataCache.resize(param.DataSize);

      const GfxParamCreateBuffer arg {
            .pResourceName = "",
            .pData = param.pData,
            .DataSize = param.DataSize,
            .bSingleUpload = false,
            .bCpuAccess = param.bCpuAccess
      };

      _buffers[0] = std::make_unique<Buffer>();
      if(!_buffers[0]->Init(arg)) {
            std::string err = "[Buffer3FCreate] Create Buffer Failed at SubBuffer 0.";
            if(!_resourceName.empty()) err += std::format(" - Name: \"{}\"", _resourceName);
            MessageManager::Log(MessageType::Error, err);
            return false;
      }
      _buffers[1] = std::make_unique<Buffer>();
      if(!_buffers[1]->Init(arg)) {
            std::string err = "[Buffer3FCreate] Create Buffer Failed at SubBuffer 1.";
            if(!_resourceName.empty()) err += std::format(" - Name: \"{}\"", _resourceName);
            MessageManager::Log(MessageType::Error, err);
            return false;
      }

      _buffers[2] = std::make_unique<Buffer>();
      if(!_buffers[2]->Init(arg)) {
            std::string err = "[Buffer3FCreate] Create Buffer Failed at SubBuffer 2.";
            if(!_resourceName.empty()) err += std::format(" - Name: \"{}\"", _resourceName);
            MessageManager::Log(MessageType::Error, err);
            return false;
      }

      return true;
}

VkBuffer Buffer3F::GetBuffer() const {
      uint32_t current_frame_index = GfxContext::Get()->GetCurrentFrameIndex();
      return _buffers[current_frame_index]->GetBuffer();
}

LoFi::Component::Gfx::Buffer* Buffer3F::GetBufferObject() const {
      uint32_t current_frame_index = GfxContext::Get()->GetCurrentFrameIndex();
      return _buffers[current_frame_index].get();
}

VkBuffer* Buffer3F::GetBufferPtr() const {
      uint32_t current_frame_index = GfxContext::Get()->GetCurrentFrameIndex();
      return _buffers[current_frame_index]->GetBufferPtr();
}

VkDeviceAddress Buffer3F::GetBufferAddress(uint32_t idx) const {
      return _buffers[idx]->GetBDAAddress();
}

VkDeviceAddress LoFi::Component::Gfx::Buffer3F::GetCurrentBufferBDAAddress() const {
      return GetBufferAddress(GfxContext::Get()->GetCurrentFrameIndex());
}

void Buffer3F::BarrierLayout(VkCommandBuffer cmd, GfxEnumKernelType new_kernel_type, GfxEnumResourceUsage new_usage) const {
      uint32_t current_frame_index = GfxContext::Get()->GetCurrentFrameIndex();
      _buffers[current_frame_index]->BarrierLayout(cmd, new_kernel_type, new_usage);
}

void Buffer3F::SetData(const void* p, uint64_t size, uint64_t offset) {
      const uint64_t copy_size =  _dataCache.size() - offset < size ? _dataCache.size() - offset : size;
      memcpy(&_dataCache.at(offset), p, copy_size);
      _dirty = 3;
      if(!_bNeedUpdate) {
            LoFi::GfxContext::Get()->EnqueueBuffer3FUpdate(GetHandle());
            _bNeedUpdate = true;
      }
}

bool Buffer3F::Update() {
      if(_dirty == 0) return false;

      const auto frame_index = GfxContext::Get()->GetCurrentFrameIndex();
      _dirty--;
      _buffers[frame_index]->SetData(_dataCache.data(), _dataCache.size());
      if(_dirty == 0) {
            _bNeedUpdate = false;
            return false;
      } else {
            return true;
      }
}

#include "Buffer.h"

#include <memory>
#include "../Message.h"
#include "../GfxContext.h"

using namespace LoFi::Internal;
using namespace LoFi::Component::Gfx;

Buffer::~Buffer() {
      Clean();
}

Buffer::Buffer(const VkBufferCreateInfo& buffer_ci, const VmaAllocationCreateInfo& alloc_ci) {
      CreateBuffer(buffer_ci, alloc_ci);
}

Buffer::Buffer(entt::entity id, const VkBufferCreateInfo& buffer_ci, const VmaAllocationCreateInfo& alloc_ci) {
      auto& world = *volkGetLoadedEcsWorld();

      if (!world.valid(id)) {
            const auto err = std::format("Buffer::Buffer - Invalid Entity ID\n");
            MessageManager::Log(MessageType::Warning, err);
            throw std::runtime_error(err);
      }

      _id = id;
      CreateBuffer(buffer_ci, alloc_ci);
}

VkDeviceAddress Buffer::GetAddress() const {
      return _address;
}

VkBufferView Buffer::CreateView(VkBufferViewCreateInfo view_ci) {
      view_ci.buffer = _buffer;

      VkBufferView view{};
      if (vkCreateBufferView(volkGetLoadedDevice(), &view_ci, nullptr, &view) != VK_SUCCESS) {
            const std::string msg = "Failed to create image view";
            MessageManager::Log(MessageType::Error, msg);
            throw std::runtime_error(msg);
      }
      _views.push_back(view);
      _viewCIs.push_back(view_ci);

      return view;
}

void Buffer::ClearViews() {
      ReleaseAllViews();
      _views.clear();
      _viewCIs.clear();
}

void* Buffer::Map() {
      if (!_mappedPtr) {
            if (vmaMapMemory(volkGetLoadedVmaAllocator(), _memory, &_mappedPtr) != VK_SUCCESS) {
                  const std::string msg = "Failed to map buffer memory";
                  MessageManager::Log(MessageType::Error, msg);
                  throw std::runtime_error(msg);
            }
      }
      return _mappedPtr;
}

void Buffer::Unmap() {
      if (_mappedPtr) {
            _mappedPtr = nullptr;
            vmaUnmapMemory(volkGetLoadedVmaAllocator(), _memory);
      }
}

void Buffer::SetData(const void* p, uint64_t size) {
      if (!p) {
            std::string msg = "Buffer::SetData Invalid data pointer";
            MessageManager::Log(MessageType::Error, msg);
            throw std::runtime_error(msg);
      }

      if (size > GetCapacity()) {
            Recreate(size);
      }

      if (IsHostSide()) {
            memcpy(Map(), p, size);
            _vaildSize = size;
      } else {
            if(size <= MAX_UPDATE_BFFER_SIZE) {
                  _dataCache.resize(size);
                  memcpy(_dataCache.data(), p, size);
                  LoFi::GfxContext::Get()->EnqueueCommand([=, this](VkCommandBuffer cmd) {
                        this->BarrierLayout(cmd, NONE, ResourceUsage::TRANS_DST);
                        vkCmdUpdateBuffer(cmd, _buffer, 0, size, _dataCache.data());
                  });
                  _vaildSize = size;
            } else {
                  if (_intermediateBuffer == nullptr) {
                        // create a upload buffer

                        auto str = std::format(R"(Buffer::SetData - Try UpLoad To device buffer, using Intermediate Buffer)");
                        MessageManager::Log(MessageType::Normal, str);

                        _intermediateBuffer = std::make_unique<Buffer>(VkBufferCreateInfo{
                              .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                              .pNext = nullptr,
                              .flags = 0,
                              .size = size,
                              .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                              .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                              .queueFamilyIndexCount = 0,
                              .pQueueFamilyIndices = nullptr
                        }, VmaAllocationCreateInfo{
                              .usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST,
                              .requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                        });
                  }

                  _intermediateBuffer->SetData(p, size);

                  auto imm_buffer = _intermediateBuffer->GetBuffer();
                  auto trans_size = size;

                  const VkBufferCopy copyinfo{
                        .srcOffset = 0,
                        .dstOffset = 0,
                        .size = trans_size
                  };

                  auto buffer = _buffer;
                  LoFi::GfxContext::Get()->EnqueueCommand([=, this](VkCommandBuffer cmd) {
                        _intermediateBuffer->BarrierLayout(cmd, NONE, ResourceUsage::TRANS_SRC);
                        this->BarrierLayout(cmd, NONE, ResourceUsage::TRANS_DST);
                        vkCmdCopyBuffer(cmd, imm_buffer, buffer, 1, &copyinfo);
                  });

                  _vaildSize = size;
            }
      }
}

void Buffer::Recreate(uint64_t size) {
      if (GetCapacity() >= size) return;

      Unmap();
      DestroyBuffer();

      _bufferCI->size = size;
      _vaildSize = 0;

      if (vmaCreateBuffer(volkGetLoadedVmaAllocator(), _bufferCI.get(), _memoryCI.get(), &_buffer, &_memory, nullptr) != VK_SUCCESS) {
            const std::string msg = "Buffer::Recreate - Failed to create buffer";
            MessageManager::Log(MessageType::Error, msg);
            throw std::runtime_error(msg);
      }

      if (IsHostSide()) Map();

      RecreateAllViews();

      const auto address_info = VkBufferDeviceAddressInfo{
            .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            .buffer = _buffer
      };
      _address = vkGetBufferDeviceAddress(volkGetLoadedDevice(), &address_info);

      auto str = std::format(R"(Buffer::CreateBuffer - Recreate "{}" bytes at "{}" side)", _bufferCI->size, _isHostSide ? "Host" : "Device");
      MessageManager::Log(MessageType::Normal, str);
}

void Buffer::BarrierLayout(VkCommandBuffer cmd, KernelType new_kernel_type, ResourceUsage new_usage) {
      if(new_kernel_type == _currentKernelType && new_usage == _currentUsage ) {
            return;
      }

      VkBufferMemoryBarrier2 barrier{};
      barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
      barrier.buffer = _buffer;
      barrier.offset = 0;
      barrier.size = VK_WHOLE_SIZE;
      barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

      if (new_kernel_type == KernelType::COMPUTE) {
            switch (new_usage) {
                  case ResourceUsage::READ_BUFFER:
                        barrier.dstAccessMask = VK_ACCESS_2_SHADER_STORAGE_READ_BIT;
                  break;
                  case ResourceUsage::WRITE_TEXTURE:
                        barrier.dstAccessMask = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
                  break;
                  case ResourceUsage::READ_WRITE_BUFFER:
                        barrier.dstAccessMask = VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
                  break;
                  default: {
                              const auto err  = std::format("Buffer::BarrierLayout - The Barrier in Buffer, In Compute kernel, resource's New Usage can't be \"{}\"\n", GetResourceUsageString(new_usage));
                              MessageManager::Log(MessageType::Error, err);
                              throw std::runtime_error(err);
                  }
            }

            barrier.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;

      } else if (new_kernel_type == KernelType::GRAPHICS) {
            switch (new_usage) {
                  case ResourceUsage::READ_BUFFER:
                        barrier.dstAccessMask = VK_ACCESS_2_SHADER_STORAGE_READ_BIT;
                        barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
                  break;
                  case ResourceUsage::WRITE_TEXTURE:
                        barrier.dstAccessMask = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
                        barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
                  break;
                  case ResourceUsage::READ_WRITE_BUFFER:
                        barrier.dstAccessMask = VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
                        barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
                  break;
                  case ResourceUsage::VERTEX_BUFFER:
                        barrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
                        barrier.dstStageMask = VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT;
                  break;
                  case ResourceUsage::INDEX_BUFFER:
                        barrier.dstAccessMask = VK_ACCESS_INDEX_READ_BIT;
                        barrier.dstStageMask = VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT;
                  break;
                  case ResourceUsage::INDIRECT_BUFFER:
                        barrier.dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
                        barrier.dstStageMask = VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;
                  break;
                  default: {
                              const auto err  = std::format("Texture::BarrierLayout - The Barrier in Buffer, In Graphics kernel, resource's New Usage can't be \"{}\"\n", GetResourceUsageString(new_usage));
                              MessageManager::Log(MessageType::Error, err);
                              throw std::runtime_error(err);
                  }
            }
      } else if (new_kernel_type == KernelType::NONE) {
            switch (new_usage) {
                  case ResourceUsage::TRANS_DST:
                        barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
                  barrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
                  break;
                  case ResourceUsage::TRANS_SRC:
                        barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
                  barrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
                  break;
                  default: {
                              const auto err  = std::format("Texture::BarrierLayout - The Barrier in Buffer, Out of Kernel, resource's New Usage can't be \"{}\"\n", GetResourceUsageString(new_usage));
                              MessageManager::Log(MessageType::Error, err);
                              throw std::runtime_error(err);
                  }
            }
      }

      //old
      if (_currentKernelType == KernelType::COMPUTE) {
            switch (_currentUsage) {
                  case ResourceUsage::READ_BUFFER:
                        barrier.srcAccessMask = VK_ACCESS_2_SHADER_STORAGE_READ_BIT;
                  break;
                  case ResourceUsage::WRITE_TEXTURE:
                        barrier.srcAccessMask = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
                  break;
                  case ResourceUsage::READ_WRITE_BUFFER:
                        barrier.srcAccessMask = VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
                  break;
                  default: {
                              const auto err  = std::format("Buffer::BarrierLayout - The Barrier in Buffer, In Compute kernel, resource's Old Usage can't be \"{}\"\n", GetResourceUsageString(new_usage));
                              MessageManager::Log(MessageType::Error, err);
                              throw std::runtime_error(err);
                  }
            }

            barrier.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;

      } else if (_currentKernelType == KernelType::GRAPHICS) {
            switch (_currentUsage) {
                  case ResourceUsage::READ_BUFFER:
                        barrier.srcAccessMask = VK_ACCESS_2_SHADER_STORAGE_READ_BIT;
                        barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
                  break;
                  case ResourceUsage::WRITE_TEXTURE:
                        barrier.srcAccessMask = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
                        barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
                  break;
                  case ResourceUsage::READ_WRITE_BUFFER:
                        barrier.srcAccessMask = VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
                        barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
                  break;
                  case ResourceUsage::VERTEX_BUFFER:
                        barrier.srcAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
                        barrier.srcStageMask = VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT;
                  break;
                  case ResourceUsage::INDEX_BUFFER:
                        barrier.srcAccessMask = VK_ACCESS_INDEX_READ_BIT;
                        barrier.srcStageMask = VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT;
                  break;
                  case ResourceUsage::INDIRECT_BUFFER:
                        barrier.srcAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
                        barrier.srcStageMask = VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;
                  break;
                  default: {
                              const auto err  = std::format("Texture::BarrierLayout - The Barrier in Buffer, In Graphics kernel, resource's Old Usage can't be \"{}\"\n", GetResourceUsageString(new_usage));
                              MessageManager::Log(MessageType::Error, err);
                              throw std::runtime_error(err);
                  }
            }
      } else if (_currentKernelType == KernelType::NONE) {
            switch (_currentUsage) {
                  case ResourceUsage::TRANS_DST:
                        barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
                        barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
                  break;
                  case ResourceUsage::TRANS_SRC:
                        barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
                        barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
                  break;
                  case ResourceUsage::NONE:
                        barrier.srcAccessMask = 0;
                        barrier.srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                  break;
                  default: {
                              const auto err  = std::format("Texture::BarrierLayout - The Barrier in Buffer, Out of Kernel, resource's Old Usage can't be \"{}\"\n", GetResourceUsageString(new_usage));
                              MessageManager::Log(MessageType::Error, err);
                              throw std::runtime_error(err);
                  }
            }
      }

      const VkDependencyInfo info {
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .bufferMemoryBarrierCount = 1,
            .pBufferMemoryBarriers = &barrier
      };

      vkCmdPipelineBarrier2(cmd, &info);

      _currentKernelType = new_kernel_type;
      _currentUsage = new_usage;
}

void Buffer::CreateBuffer(const VkBufferCreateInfo& buffer_ci, const VmaAllocationCreateInfo& alloc_ci) {
      _bufferCI = std::make_unique<VkBufferCreateInfo>(buffer_ci);
      _memoryCI = std::make_unique<VmaAllocationCreateInfo>(alloc_ci);

      if (_buffer != nullptr) {
            DestroyBuffer();
      }

      if (vmaCreateBuffer(volkGetLoadedVmaAllocator(), _bufferCI.get(), _memoryCI.get(), &_buffer, &_memory, nullptr) != VK_SUCCESS) {
            const std::string msg = "Buffer::CreateBuffer - Failed to create buffer";
            MessageManager::Log(MessageType::Error, msg);
            throw std::runtime_error(msg);
      }

      VkMemoryPropertyFlags fgs;
      vmaGetAllocationMemoryProperties(volkGetLoadedVmaAllocator(), _memory, &fgs);
      _isHostSide = (fgs & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0;

      _vaildSize = 0;

      const auto address_info = VkBufferDeviceAddressInfo{
            .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            .buffer = _buffer
      };
      _address = vkGetBufferDeviceAddress(volkGetLoadedDevice(), &address_info);

      auto str = std::format(R"(Buffer::CreateBuffer - Emplace "{}" bytes at "{}" side)", _bufferCI->size, _isHostSide ? "Host" : "Device");
      MessageManager::Log(MessageType::Normal, str);
}

void Buffer::ReleaseAllViews() const {
      for (const auto view : _views) {
            ContextResourceRecoveryInfo info{
                  .Type = ContextResourceType::BUFFER_VIEW,
                  .Resource1 = (size_t)view
            };
            GfxContext::Get()->RecoveryContextResource(info);
      }
}

void Buffer::RecreateAllViews() {
      ReleaseAllViews();
      _views.clear();
      CreateViewFromCurrentViewCIs();
}

void Buffer::CreateViewFromCurrentViewCIs() {
      for (auto& view_ci : _viewCIs) {
            VkBufferView view{};
            view_ci.buffer = _buffer;
            if (vkCreateBufferView(volkGetLoadedDevice(), &view_ci, nullptr, &view) != VK_SUCCESS) {
                  const std::string msg = "Buffer::CreateViewFromCurrentViewCIs - Failed to create buffer view";
                  MessageManager::Log(MessageType::Error, msg);
                  throw std::runtime_error(msg);
            }
      }
}

void Buffer::Clean() {
      ClearViews();
      DestroyBuffer();
      _vaildSize = 0;
      _address = 0;
}

void Buffer::DestroyBuffer() {
      ContextResourceRecoveryInfo info{
            .Type = ContextResourceType::BUFFER,
            .Resource1 = (size_t)_buffer,
            .Resource2 = (size_t)_memory,
      };
      GfxContext::Get()->RecoveryContextResource(info);
}

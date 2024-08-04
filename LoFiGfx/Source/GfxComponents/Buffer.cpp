#include "Buffer.h"

#include <memory>
#include "../Message.h"
#include "../GfxContext.h"

using namespace LoFi::Internal;
using namespace LoFi::Component::Gfx;

Buffer::~Buffer() {
      Clean();
}

Buffer::Buffer() = default;

Buffer::Buffer(entt::entity id) : _id(id) {}

bool Buffer::Init(const GfxParamCreateBuffer& param) {
      _resourceName = param.pResourceName ? param.pResourceName : std::string{};
      if(param.DataSize == 0) {
            std::string err = "[Buffer::Init] Create Buffer Failed! DataSize is 0.";
            if (!_resourceName.empty()) err += std::format(" - Name: \"{}\"", _resourceName);
            MessageManager::Log(MessageType::Error, err);
            return false;
      }

      VkBufferCreateInfo buffer_ci{};
      buffer_ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
      buffer_ci.size = param.DataSize;
      buffer_ci.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT |
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
      buffer_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

      VmaAllocationCreateInfo alloc_ci{};
      alloc_ci.usage = param.bCpuAccess ? VMA_MEMORY_USAGE_AUTO_PREFER_HOST : VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;


      _bufferCI = std::make_unique<VkBufferCreateInfo>(buffer_ci);
      _memoryCI = std::make_unique<VmaAllocationCreateInfo>(alloc_ci);
      if(!CreateBuffer()) {
            return false;
      }

      if(param.pData != nullptr) {
            if(!this->SetData(param.pData, param.DataSize)) {
                  std::string err = "[Buffer::Init] Create Buffer Create Success! But SetData Failed.";
                  if (!_resourceName.empty()) err += std::format(" - Name: \"{}\"", _resourceName);
                  MessageManager::Log(MessageType::Warning, err);
            }
      }
      return true;
}

bool Buffer::Init(const char* name, const VkBufferCreateInfo& ci, const VmaAllocationCreateInfo& mem_ci) {
      _resourceName = name ? name : std::string{};
      if(ci.size == 0) {
            std::string err = "[Buffer::Init] Create Buffer Failed! DataSize is 0.";
            if (!_resourceName.empty()) err += std::format(" - Name: \"{}\"", _resourceName);
            MessageManager::Log(MessageType::Error, err);
            return false;
      }

      _bufferCI = std::make_unique<VkBufferCreateInfo>(ci);
      _memoryCI = std::make_unique<VmaAllocationCreateInfo>(mem_ci);

     return CreateBuffer();
}

VkDeviceAddress Buffer::GetBDAAddress() const {
      return _address;
}

VkBufferView Buffer::CreateView(VkBufferViewCreateInfo view_ci) {
      view_ci.buffer = _buffer;

      VkBufferView view{};
      if (const auto res = vkCreateBufferView(volkGetLoadedDevice(), &view_ci, nullptr, &view); res != VK_SUCCESS) {
            auto err = std::format("[Buffer::CreateView] vkCreateBufferView Failed, return {}.", ToStringVkResult(res));
            if (!_resourceName.empty())
                  err += std::format(" - Name: \"{}\"", _resourceName);
            MessageManager::Log(MessageType::Error, err);
            return nullptr;
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
            if (const auto res = vmaMapMemory(volkGetLoadedVmaAllocator(), _memory, &_mappedPtr); res != VK_SUCCESS) {
                  std::string err = std::format("[Buffer::Map] Failed to Map Buffer's Memory, Maybe it's not a Host Visible Buffer. Vulkan return {}.", ToStringVkResult(res));
                  if (!_resourceName.empty())
                        err += std::format(" - Name: \"{}\"", _resourceName);
                  MessageManager::Log(MessageType::Error, err);
                  return nullptr;
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

bool Buffer::SetData(const void* p, uint64_t size, uint8_t recursive_depth) {
      if(recursive_depth > 1) {
            std::string err = "[Buffer::SetData] Create Buffer Failed! Recursive Depth > 1.";
            if (!_resourceName.empty()) err += std::format(" - Name: \"{}\"", _resourceName);
            MessageManager::Log(MessageType::Fatal, err);
            throw std::runtime_error(err);
      }

      if (!p) {
            std::string err = "[Buffer::SetData] Create Buffer Failed! Maybe Resource is too much, can't allocate any more resource.";
            if (!_resourceName.empty()) err += std::format(" - Name: \"{}\"", _resourceName);
            MessageManager::Log(MessageType::Error, err);
            return false;
      }

      if (size > GetCapacity()) {
            Recreate(size);
      }

      if (IsHostSide()) {
            memcpy(Map(), p, size);
      } else {
            if (size <= MAX_UPDATE_BFFER_SIZE) {
                  _dataCache.resize(size);
                  memcpy(_dataCache.data(), p, size);
                  _updateCommand = [=, this](VkCommandBuffer cmd) {
                        this->BarrierLayout(cmd, GfxEnumKernelType::OUT_OF_KERNEL, GfxEnumResourceUsage::TRANS_DST);
                        vkCmdUpdateBuffer(cmd, _buffer, 0, size, _dataCache.data());
                  };
            } else {
                  if (_intermediateBuffer == nullptr) {
                        // create a upload buffer

                        auto str = std::format(R"([Buffer::SetData] Try UpLoad To Device Buffer. Create an Intermediate Buffer {} Bytes)", size);
                        if (!_resourceName.empty())str += std::format(" - Name: \"{}\"", _resourceName);
                        MessageManager::Log(MessageType::Normal, str);
                        _intermediateBuffer = std::make_unique<Buffer>();

                        std::string buffer_name = std::format("Buffer[{}]Upload Intermediate Buffer", _resourceName);
                        if(!_intermediateBuffer->Init(buffer_name.c_str(), VkBufferCreateInfo{
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
                        })) {
                              return false;
                        }
                  }

                  _intermediateBuffer->SetData(p, size, recursive_depth + 1);

                  auto imm_buffer = _intermediateBuffer->GetBuffer();
                  auto trans_size = size;

                  const VkBufferCopy copyinfo{
                        .srcOffset = 0,
                        .dstOffset = 0,
                        .size = trans_size
                  };

                  auto buffer = _buffer;
                  _updateCommand = [=, this](VkCommandBuffer cmd) {
                        _intermediateBuffer->BarrierLayout(cmd, GfxEnumKernelType::OUT_OF_KERNEL, GfxEnumResourceUsage::TRANS_SRC);
                        this->BarrierLayout(cmd, GfxEnumKernelType::OUT_OF_KERNEL, GfxEnumResourceUsage::TRANS_DST);
                        vkCmdCopyBuffer(cmd, imm_buffer, buffer, 1, &copyinfo);
                  };
            }

            if(!_bNeedUpdate) {
                  _bNeedUpdate = true;
                  LoFi::GfxContext::Get()->EnqueueBufferUpdate(GetHandle());
            }
      }
      return true;
}

bool Buffer::Recreate(uint64_t size) {
      if (GetCapacity() >= size) return true;

      size_t back_up_size = _bufferCI->size;
      _bufferCI->size = size;

      VkBuffer new_buffer{};
      VmaAllocation new_mem{};
      if (const auto res = vmaCreateBuffer(volkGetLoadedVmaAllocator(), _bufferCI.get(), _memoryCI.get(), &new_buffer, &new_mem, nullptr); res != VK_SUCCESS) {
            auto err = std::format("[Buffer::Recreate] Failed To Recreate Buffer (from {} Bytes to {} Bytes) , vmaCreateBuffer return {}.", back_up_size, size, ToStringVkResult(res));
            if (!_resourceName.empty()) err += std::format(" - Name: \"{}\"", _resourceName);
            MessageManager::Log(MessageType::Error, err);
            _bufferCI->size = back_up_size;
            return false;
      }

      Unmap();
      DestroyBuffer();
      _buffer = new_buffer;
      _memory = new_mem;

      if (IsHostSide()) Map();

      RecreateAllViews();

      const auto address_info = VkBufferDeviceAddressInfo{
            .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            .buffer = _buffer
      };
      _address = vkGetBufferDeviceAddress(volkGetLoadedDevice(), &address_info);

      auto str = std::format(R"([Buffer::Recreate] Recreate Buffer from "{}" to "{}" bytes at "{}" side)", back_up_size, _bufferCI->size, _isHostSide ? "Host" : "Device");
      if (!_resourceName.empty())
            str += std::format(" - Name: \"{}\"", _resourceName);
      MessageManager::Log(MessageType::Normal, str);

      return true;
}

void Buffer::BarrierLayout(VkCommandBuffer cmd, GfxEnumKernelType new_kernel_type, GfxEnumResourceUsage new_usage) {
      if (new_kernel_type == _currentKernelType && new_usage == _currentUsage) {
            return;
      }

      VkBufferMemoryBarrier2 barrier{};
      barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
      barrier.buffer = _buffer;
      barrier.offset = 0;
      barrier.size = VK_WHOLE_SIZE;
      barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

      if (new_kernel_type == GfxEnumKernelType::COMPUTE) {
            switch (new_usage) {
                  case GfxEnumResourceUsage::READ_BUFFER:
                        barrier.dstAccessMask = VK_ACCESS_2_SHADER_STORAGE_READ_BIT;
                        break;
                  case GfxEnumResourceUsage::WRITE_BUFFER:
                        barrier.dstAccessMask = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
                        break;
                  case GfxEnumResourceUsage::READ_WRITE_BUFFER:
                        barrier.dstAccessMask = VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
                        break;
                  default:
                        {
                              auto err = std::format("[Buffer::BarrierLayout] The Barrier in Buffer, In Compute kernel, New Usage can't be \"{}\".", ToStringResourceUsage(new_usage));
                              if (!_resourceName.empty())
                                    err += std::format(" - Name: \"{}\"", _resourceName);
                              MessageManager::Log(MessageType::Error, err);
                              throw std::runtime_error(err);
                        }
            }
            barrier.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
      } else if (new_kernel_type == GfxEnumKernelType::GRAPHICS) {
            switch (new_usage) {
                  case GfxEnumResourceUsage::READ_BUFFER:
                        barrier.dstAccessMask = VK_ACCESS_2_SHADER_STORAGE_READ_BIT;
                        barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
                        break;
                  case GfxEnumResourceUsage::WRITE_BUFFER:
                        barrier.dstAccessMask = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
                        barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
                        break;
                  case GfxEnumResourceUsage::READ_WRITE_BUFFER:
                        barrier.dstAccessMask = VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
                        barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
                        break;
                  case GfxEnumResourceUsage::VERTEX_BUFFER:
                        barrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
                        barrier.dstStageMask = VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT;
                        break;
                  case GfxEnumResourceUsage::INDEX_BUFFER:
                        barrier.dstAccessMask = VK_ACCESS_INDEX_READ_BIT;
                        barrier.dstStageMask = VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT;
                        break;
                  case GfxEnumResourceUsage::INDIRECT_BUFFER:
                        barrier.dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
                        barrier.dstStageMask = VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;
                        break;
                  default:
                        {
                              auto err = std::format("[Buffer::BarrierLayout] The Barrier in Buffer, In Compute kernel, New Usage can't be \"{}\".", ToStringResourceUsage(new_usage));
                              if (!_resourceName.empty())
                                    err += std::format(" - Name: \"{}\"", _resourceName);
                              MessageManager::Log(MessageType::Error, err);
                              throw std::runtime_error(err);
                        }
            }
      } else if (new_kernel_type == GfxEnumKernelType::OUT_OF_KERNEL) {
            switch (new_usage) {
                  case GfxEnumResourceUsage::TRANS_DST:
                        barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
                        barrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
                        break;
                  case GfxEnumResourceUsage::TRANS_SRC:
                        barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
                        barrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
                        break;
                  default:
                        {
                              auto err = std::format("[Buffer::BarrierLayout] The Barrier in Buffer, In Compute kernel, New Usage can't be \"{}\".", ToStringResourceUsage(new_usage));
                              if (!_resourceName.empty())
                                    err += std::format(" - Name: \"{}\"", _resourceName);
                              MessageManager::Log(MessageType::Error, err);
                              throw std::runtime_error(err);
                        }
            }
      }

      //old
      if (_currentKernelType == GfxEnumKernelType::COMPUTE) {
            switch (_currentUsage) {
                  case GfxEnumResourceUsage::READ_BUFFER:
                        barrier.srcAccessMask = VK_ACCESS_2_SHADER_STORAGE_READ_BIT;
                        barrier.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
                        break;
                  case GfxEnumResourceUsage::WRITE_BUFFER:
                        barrier.srcAccessMask = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
                        barrier.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
                        break;
                  case GfxEnumResourceUsage::READ_WRITE_BUFFER:
                        barrier.srcAccessMask = VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
                        barrier.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
                        break;
                  default:
                        {
                              auto err = std::format("[Buffer::BarrierLayout] The Barrier in Buffer, In Compute kernel, Old Usage can't be \"{}\".", ToStringResourceUsage(new_usage));
                              if (!_resourceName.empty())
                                    err += std::format(" - Name: \"{}\"", _resourceName);
                              MessageManager::Log(MessageType::Error, err);
                              throw std::runtime_error(err);
                        }
            }
      } else if (_currentKernelType == GfxEnumKernelType::GRAPHICS) {
            switch (_currentUsage) {
                  case GfxEnumResourceUsage::READ_BUFFER:
                        barrier.srcAccessMask = VK_ACCESS_2_SHADER_STORAGE_READ_BIT;
                        barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
                        break;
                  case GfxEnumResourceUsage::WRITE_BUFFER:
                        barrier.srcAccessMask = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
                        barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
                        break;
                  case GfxEnumResourceUsage::READ_WRITE_BUFFER:
                        barrier.srcAccessMask = VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
                        barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
                        break;
                  case GfxEnumResourceUsage::VERTEX_BUFFER:
                        barrier.srcAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
                        barrier.srcStageMask = VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT;
                        break;
                  case GfxEnumResourceUsage::INDEX_BUFFER:
                        barrier.srcAccessMask = VK_ACCESS_INDEX_READ_BIT;
                        barrier.srcStageMask = VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT;
                        break;
                  case GfxEnumResourceUsage::INDIRECT_BUFFER:
                        barrier.srcAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
                        barrier.srcStageMask = VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;
                        break;
                  default:
                        {
                              auto err = std::format("[Buffer::BarrierLayout] The Barrier in Buffer, In Compute kernel, Old Usage can't be \"{}\".", ToStringResourceUsage(new_usage));
                              if (!_resourceName.empty())
                                    err += std::format(" - Name: \"{}\"", _resourceName);
                              MessageManager::Log(MessageType::Error, err);
                              throw std::runtime_error(err);
                        }
            }
      } else if (_currentKernelType == GfxEnumKernelType::OUT_OF_KERNEL) {
            switch (_currentUsage) {
                  case GfxEnumResourceUsage::TRANS_DST:
                        barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
                        barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
                        break;
                  case GfxEnumResourceUsage::TRANS_SRC:
                        barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
                        barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
                        break;
                  case GfxEnumResourceUsage::UNKNOWN_RESOURCE_USAGE:
                        barrier.srcAccessMask = 0;
                        barrier.srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                        break;
                  default:
                        {
                              auto err = std::format("[Buffer::BarrierLayout] The Barrier in Buffer, In Compute kernel, Old Usage can't be \"{}\".", ToStringResourceUsage(new_usage));
                              if (!_resourceName.empty())
                                    err += std::format(" - Name: \"{}\"", _resourceName);
                              MessageManager::Log(MessageType::Error, err);
                              throw std::runtime_error(err);
                        }
            }
      }

      const VkDependencyInfo info{
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .bufferMemoryBarrierCount = 1,
            .pBufferMemoryBarriers = &barrier
      };

      vkCmdPipelineBarrier2(cmd, &info);

      _currentKernelType = new_kernel_type;
      _currentUsage = new_usage;
}

void Buffer::SetLayout(GfxEnumKernelType new_kernel_type, GfxEnumResourceUsage new_usage) {
      _currentUsage = new_usage;
      _currentKernelType = new_kernel_type;
}

bool Buffer::CreateBuffer() {

      if (_buffer != nullptr) {
            // wtf
            std::string err = "[Buffer::CreateBuffer] Create Buffer Failed! Buffer is not nullptr? WTF????.";
            if (!_resourceName.empty()) err += std::format(" - Name: \"{}\"", _resourceName);
            MessageManager::Log(MessageType::Fatal, err);
            return false;
      }

      if (const auto res = vmaCreateBuffer(volkGetLoadedVmaAllocator(), _bufferCI.get(), _memoryCI.get(), &_buffer, &_memory, nullptr); res != VK_SUCCESS) {
            std::string err = std::format("[Buffer::CreateBuffer] vmaCreateBuffer Failed, return {}.", ToStringVkResult(res));
            if (!_resourceName.empty())
                  err += std::format(" - Name: \"{}\"", _resourceName);
            MessageManager::Log(MessageType::Error, err);
            return false;
      }

      VkMemoryPropertyFlags fgs;
      vmaGetAllocationMemoryProperties(volkGetLoadedVmaAllocator(), _memory, &fgs);
      _isHostSide = (fgs & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0;

      const auto address_info = VkBufferDeviceAddressInfo{
            .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            .buffer = _buffer
      };
      _address = vkGetBufferDeviceAddress(volkGetLoadedDevice(), &address_info);

      auto str = std::format(R"([Buffer::CreateBuffer] Emplace "{}" bytes at "{}" side)", _bufferCI->size, _isHostSide ? "Host" : "Device");
      if (!_resourceName.empty())
            str += std::format(" - Name: \"{}\"", _resourceName);
      MessageManager::Log(MessageType::Normal, str);

      return true;
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
            if (const auto res = vkCreateBufferView(volkGetLoadedDevice(), &view_ci, nullptr, &view); res != VK_SUCCESS) {
                  auto err = std::format("[Buffer::CreateViewFromCurrentViewCIs] vkCreateBufferView Failed, return {}.", ToStringVkResult(res));
                  if (!_resourceName.empty())
                        err += std::format(" - Name: \"{}\"", _resourceName);
                  MessageManager::Log(MessageType::Error, err);
                  throw std::runtime_error(err);
            }
      }
}

void Buffer::Clean() {
      ClearViews();
      DestroyBuffer();
      _address = 0;
}

void Buffer::DestroyBuffer() {
      std::string str = std::format("[Buffer::DestroyBuffer] Destroy Buffer at {} side, Buffer: {}.", _isHostSide ? "Host" : "Device", _resourceName);
      MessageManager::Log(MessageType::Normal, str);
      Unmap();
      if(_buffer && _memory) {
            ContextResourceRecoveryInfo info{
                  .Type = ContextResourceType::BUFFER,
                  .Resource1 = (size_t)_buffer,
                  .Resource2 = (size_t)_memory,
                  .ResourceName = _resourceName
            };
            GfxContext::Get()->RecoveryContextResource(info);
      }
}

void Buffer::Update(VkCommandBuffer cmd) {
      if (_updateCommand) {
            _updateCommand(cmd);
            _updateCommand = nullptr;
            _bNeedUpdate = false;
      }
}
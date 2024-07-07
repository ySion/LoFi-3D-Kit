#include "Buffer.h"
#include "../Message.h"
#include "../Context.h"

using namespace LoFi::Internal;
using namespace LoFi::Component;

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
      const auto address_info = VkBufferDeviceAddressInfo{
            .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            .buffer = _buffer
      };
      const VkDeviceAddress address = vkGetBufferDeviceAddress(volkGetLoadedDevice(), &address_info);
      return address;
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
            if(size <= 1024 * 4) {
                  LoFi::Context::Get()->EnqueueCommand([=](VkCommandBuffer cmd) {
                        vkCmdUpdateBuffer(cmd, _buffer, 0, size, p);
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
                              .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
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
                  LoFi::Context::Get()->EnqueueCommand([ =](VkCommandBuffer cmd) {
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

      auto str = std::format(R"(Buffer::CreateBuffer - Recreate "{}" bytes at "{}" side)", _bufferCI->size, _isHostSide ? "Host" : "Device");
      MessageManager::Log(MessageType::Normal, str);
}

void Buffer::BarrierLayout(VkCommandBuffer cmd, VkAccessFlags2 new_access, std::optional<VkAccessFlags2> src_layout,
      std::optional<VkPipelineStageFlags2> src_stage, std::optional<VkPipelineStageFlags2> dst_stage) {

      if(new_access == _currentAccess) { return; }

      VkMemoryBarrier2 barrier {
            .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2_KHR,
            .srcStageMask = src_stage.value_or(VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT),
            .dstStageMask = dst_stage.value_or(VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT),
      };

      barrier.srcAccessMask = _currentAccess;
      barrier.dstAccessMask = new_access;
      _currentAccess = new_access;

      const VkDependencyInfo info{
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .memoryBarrierCount = 1,
            .pMemoryBarriers = &barrier
      };

      vkCmdPipelineBarrier2(cmd, &info);
}

void Buffer::CreateBuffer(const VkBufferCreateInfo& buffer_ci, const VmaAllocationCreateInfo& alloc_ci) {
      _bufferCI.reset(new VkBufferCreateInfo{buffer_ci});
      _memoryCI.reset(new VmaAllocationCreateInfo{alloc_ci});

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

      auto str = std::format(R"(Buffer::CreateBuffer - Emplace "{}" bytes at "{}" side)", _bufferCI->size, _isHostSide ? "Host" : "Device");
      MessageManager::Log(MessageType::Normal, str);
}

void Buffer::ReleaseAllViews() const {
      for (const auto view : _views) {
            ContextResourceRecoveryInfo info{
                  .Type = ContextResourceType::BUFFER_VIEW,
                  .Resource1 = (size_t)view
            };
            Context::Get()->RecoveryContextResource(info);
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
}

void Buffer::DestroyBuffer() {
      ContextResourceRecoveryInfo info{
            .Type = ContextResourceType::BUFFER,
            .Resource1 = (size_t)_buffer,
            .Resource2 = (size_t)_memory,
      };
      Context::Get()->RecoveryContextResource(info);
}

#include "Buffer.h"
#include "../Message.h"
#include "../Context.h"

using namespace LoFi::Internal;

LoFi::Component::Buffer::~Buffer() {
      Clean();
}

LoFi::Component::Buffer::Buffer(const VkBufferCreateInfo& buffer_ci, const VmaAllocationCreateInfo& alloc_ci) {
      CreateBuffer(buffer_ci, alloc_ci);
}

LoFi::Component::Buffer::Buffer(entt::entity id, const VkBufferCreateInfo& buffer_ci,
const VmaAllocationCreateInfo& alloc_ci) {
      _id = id;
      CreateBuffer(buffer_ci, alloc_ci);
}

VkBufferView LoFi::Component::Buffer::CreateView(VkBufferViewCreateInfo view_ci) {
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

void LoFi::Component::Buffer::ClearViews() {
      ReleaseAllViews();
      _views.clear();
      _viewCIs.clear();
}

void* LoFi::Component::Buffer::Map() {
      if (!_mappedPtr) {
            if (vmaMapMemory(volkGetLoadedVmaAllocator(), _memory, &_mappedPtr) != VK_SUCCESS) {
                  const std::string msg = "Failed to map buffer memory";
                  MessageManager::Log(MessageType::Error, msg);
                  throw std::runtime_error(msg);
            }
      }
      return _mappedPtr;
}

void LoFi::Component::Buffer::Unmap() {
      if (_mappedPtr) {
            _mappedPtr = nullptr;
            vmaUnmapMemory(volkGetLoadedVmaAllocator(), _memory);
      }
}

void LoFi::Component::Buffer::SetData(const void* p, uint64_t size) {
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

            LoFi::Context::Get()->EnqueueCommand([=](VkCommandBuffer cmd) {
                  vkCmdCopyBuffer(cmd, imm_buffer, _buffer, 1, &copyinfo);
            });

            _vaildSize = size;
      }
}

void LoFi::Component::Buffer::Recreate(uint64_t size) {
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

      if (_bindlessIndex.has_value()) {
            _bindlessIndex = Context::Get()->MakeBindlessIndexBuffer(_id);
      }

      auto str = std::format(R"(Buffer::CreateBuffer - Recreate "{}" bytes at "{}" side)", _bufferCI->size, _isHostSide ? "Host" : "Device");
      MessageManager::Log(MessageType::Normal, str);
}

void LoFi::Component::Buffer::CreateBuffer(const VkBufferCreateInfo& buffer_ci, const VmaAllocationCreateInfo& alloc_ci) {
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

void LoFi::Component::Buffer::ReleaseAllViews() const {
      for (const auto view : _views) {
            ContextResourceRecoveryInfo info{
                  .Type = ContextResourceType::BUFFERVIEW,
                  .Resource1 = (size_t)view
            };
            Context::Get()->RecoveryContextResource(info);
      }
}

void LoFi::Component::Buffer::RecreateAllViews() {
      ReleaseAllViews();
      _views.clear();
      CreateViewFromCurrentViewCIs();
}

void LoFi::Component::Buffer::CreateViewFromCurrentViewCIs() {
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

void LoFi::Component::Buffer::Clean() {
      ClearViews();
      DestroyBuffer();
      _vaildSize = 0;
}

void LoFi::Component::Buffer::SetBindlessIndex(std::optional<uint32_t> bindless_index) {
      _bindlessIndex = bindless_index;
}

void LoFi::Component::Buffer::DestroyBuffer() {
      ContextResourceRecoveryInfo info{
            .Type = ContextResourceType::BUFFER,
            .Resource1 = (size_t)_buffer,
            .Resource2 = (size_t)_memory,
            .Resource3 = _bindlessIndex
      };
      Context::Get()->RecoveryContextResource(info);
}

#include "Texture.h"

#include <memory>
#include "../Message.h"
#include "../Context.h"

using namespace LoFi::Internal;
using namespace LoFi::Component;

Texture::~Texture() {
      Clean();
}

Texture::Texture(const VkImageCreateInfo& image_ci, VkImage Image, bool isBorrow) {
      _id = entt::null;
      _isBorrow = isBorrow;

      _image = Image;
      _imageCI = std::make_unique<VkImageCreateInfo>(image_ci);

      _currentLayout = _imageCI->initialLayout;
}

Texture::Texture(entt::entity id, const VkImageCreateInfo& image_ci, const VmaAllocationCreateInfo& alloc_ci) {
      auto& world = *volkGetLoadedEcsWorld();

      if (!world.valid(id)) {
            const auto err = std::format("Texture::Texture - Invalid Entity ID\n");
            MessageManager::Log(MessageType::Warning, err);
            throw std::runtime_error(err);
      }

      const auto allocator = volkGetLoadedVmaAllocator();
      _imageCI = std::make_unique<VkImageCreateInfo>(image_ci);
      if (vmaCreateImage(allocator, &image_ci, &alloc_ci, &_image, &_memory, nullptr) != VK_SUCCESS) {
            const std::string msg = "Failed to create image";
            MessageManager::Log(MessageType::Error, msg);
            throw std::runtime_error(msg);
      }
      _id = id;
      _currentLayout = _imageCI->initialLayout;
}

VkImageView Texture::CreateView(VkImageViewCreateInfo view_ci) {
      view_ci.image = _image;

      VkImageView view{};

      if (vkCreateImageView(volkGetLoadedDevice(), &view_ci, nullptr, &view) != VK_SUCCESS) {
            const std::string msg = "Failed to create image view";
            MessageManager::Log(MessageType::Error, msg);
            throw std::runtime_error(msg);
      }

      _views.push_back(view);
      _viewCIs.push_back(view_ci);

      return view;
}

void Texture::ClearViews() {
      ReleaseAllViews();
      _views.clear();
      _viewCIs.clear();
}

void Texture::SetData(const void* data, size_t size) {
      VmaAllocationInfo info{};
      vmaGetAllocationInfo(volkGetLoadedVmaAllocator(), _memory, &info);

      if (info.size < size) {
            auto str = std::format(R"(Texture::SetData - Size Mismatch, Expected: {} {}, Actual: {})", info.size, size);
            MessageManager::Log(MessageType::Error, str);
            return;
      }

      if (_intermediateBuffer == nullptr) {
            // create a upload buffer

            auto str = std::format(R"(Texture::SetData - Try UpLoad To device texture, using Intermediate Buffer)");
            MessageManager::Log(MessageType::Normal, str);

            _intermediateBuffer = std::make_unique<Buffer>(VkBufferCreateInfo{
                  .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                  .pNext = nullptr,
                  .flags = 0,
                  .size = info.size,
                  .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                  .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                  .queueFamilyIndexCount = 0,
                  .pQueueFamilyIndices = nullptr
            }, VmaAllocationCreateInfo{
                  .usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST,
                  .requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
            });
      }

      _intermediateBuffer->SetData(data, size);

      auto imm_buffer = _intermediateBuffer->GetBuffer();
      auto trans_size = size;

      VkBufferImageCopy buffer_copyto_image{
            .bufferOffset = 0,
            .bufferRowLength = 0,
            .bufferImageHeight = 0,
            .imageSubresource = {
                  .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                  .mipLevel = 0,
                  .baseArrayLayer = 0,
                  .layerCount = 1
            },
            .imageOffset = {},
            .imageExtent = {
                  .width = _imageCI->extent.width,
                  .height = _imageCI->extent.height,
                  .depth = 1
            }
      };

      LoFi::Context::Get()->EnqueueCommand([=, this](VkCommandBuffer cmd) {
            this->BarrierLayout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            vkCmdCopyBufferToImage(cmd, imm_buffer, _image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &buffer_copyto_image);
      });
}

void Texture::BarrierLayout(VkCommandBuffer cmd, VkImageLayout new_layout, std::optional<VkImageLayout> src_layout,
std::optional<VkPipelineStageFlags2> src_stage, std::optional<VkPipelineStageFlags2> dst_stage) {
      auto& old_layout = _currentLayout;

      if (old_layout == new_layout) return;

      VkImageMemoryBarrier2 barrier{};
      barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR;
      barrier.image = _image;
      barrier.subresourceRange = {
            .aspectMask = _viewCIs.at(0).subresourceRange.aspectMask,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
      };

      barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

      barrier.oldLayout = _currentLayout;
      barrier.newLayout = new_layout;

      barrier.srcStageMask = src_stage.value_or(VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT);
      barrier.dstStageMask = dst_stage.value_or(VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT);

      barrier.srcAccessMask = 0;
      barrier.dstAccessMask = 0;

      if (src_layout.has_value()) {
            old_layout = src_layout.value();
      }

      switch (old_layout) {
            case VK_IMAGE_LAYOUT_UNDEFINED:
            case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
                  barrier.srcAccessMask = 0;
                  break;

            case VK_IMAGE_LAYOUT_PREINITIALIZED:
                  barrier.srcAccessMask = VK_ACCESS_2_HOST_WRITE_BIT;
                  break;

            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                  barrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
                  break;

            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            case VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL:
            case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
                  barrier.srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                  break;

            case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                  barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
                  break;

            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                  barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
                  break;

            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                  barrier.srcAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
                  break;

            default: break;
      }

      switch (new_layout) {
            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                  barrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
                  break;
            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            case VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL:
            case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
                  barrier.dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                  break;
            case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                  barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
                  break;
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                  barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
                  break;
            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                  if (barrier.srcAccessMask == 0) {
                        barrier.srcAccessMask = VK_ACCESS_2_HOST_WRITE_BIT | VK_ACCESS_2_TRANSFER_WRITE_BIT;
                  }
                  barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
                  break;
            case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
                  barrier.dstAccessMask = 0;
                  break;
            default: break;
      }

      old_layout = barrier.newLayout;

      const VkDependencyInfo info{
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &barrier
      };

      vkCmdPipelineBarrier2(cmd, &info);
}


void Texture::ReleaseAllViews() const {
      for (const auto view : _views) {
            ContextResourceRecoveryInfo info{
                  .Type = ContextResourceType::IMAGE_VIEW,
                  .Resource1 = (size_t)view
            };
            Context::Get()->RecoveryContextResource(info);
      }
}

void Texture::Clean() {
      ClearViews();

      _currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;

      if (!_isBorrow) {
            DestroyTexture();
      }
}

void Texture::SetBindlessIndexForSampler(std::optional<uint32_t> index) {
      _bindlessIndexForSampler = index;
}

void Texture::SetBindlessIndexForComputeKernel(std::optional<uint32_t> index) {
      _bindlessIndexForComputeKernel = index;
}

void Texture::DestroyTexture() {
      ContextResourceRecoveryInfo info{
            .Type = ContextResourceType::IMAGE,
            .Resource1 = (size_t)_image,
            .Resource2 = (size_t)_memory,
            .Resource3 = _bindlessIndexForComputeKernel,
            .Resource4 = _bindlessIndexForSampler
      };
      Context::Get()->RecoveryContextResource(info);
}

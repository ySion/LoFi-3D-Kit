#include "Texture.h"

#include "../Message.h"
#include "../GfxContext.h"

using namespace LoFi::Internal;
using namespace LoFi::Component::Gfx;

Texture::~Texture() {
      Clean();
}

Texture::Texture(const VkImageCreateInfo& image_ci, VkImage Image, bool isBorrow) {
      _id = entt::null;
      _isBorrow = isBorrow;

      _image = Image;
      _imageCI = std::make_unique<VkImageCreateInfo>(image_ci);
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
            const std::string msg = "Texture::Texture - Failed to create image";
            MessageManager::Log(MessageType::Error, msg);
            throw std::runtime_error(msg);
      }
      _memoryCI = std::make_unique<VmaAllocationCreateInfo>(alloc_ci);
      _id = id;
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

void Texture::Resize(uint32_t w, uint32_t h) {
      if(_imageCI->extent.width == w && _imageCI->extent.height == h) return;

      //remove all views
      //remove images
      ReleaseAllViews();
      DestroyTexture();

      auto& world = *volkGetLoadedEcsWorld();
      const auto allocator = volkGetLoadedVmaAllocator();

      _imageCI->extent.width = w;
      _imageCI->extent.width = h;
      if (vmaCreateImage(allocator, _imageCI.get(), _memoryCI.get(), &_image, &_memory, nullptr) != VK_SUCCESS) {
            const std::string msg = "Texture::Resize - Failed to Resize image";
            MessageManager::Log(MessageType::Error, msg);
            throw std::runtime_error(msg);
      }

      for (auto& view_ci : _viewCIs) {
            view_ci.image = _image;
            CreateView(view_ci);
      }

}

void Texture::SetData(const void* data, size_t size) {
      VmaAllocationInfo info{};
      vmaGetAllocationInfo(volkGetLoadedVmaAllocator(), _memory, &info);

      if (info.size < size) {
            auto str = std::format(R"(Texture::SetData - Size Mismatch, Expected: {}, Actual: {})", info.size, size);
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
                  .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
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
      //auto trans_size = size;

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

      _updateCommand = [=, this](VkCommandBuffer cmd) {
            this->BarrierLayout(cmd, NONE, ResourceUsage::TRANS_DST);
            _intermediateBuffer->BarrierLayout(cmd, NONE, ResourceUsage::TRANS_SRC);
            vkCmdCopyBufferToImage(cmd, imm_buffer, _image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &buffer_copyto_image);
      };

      auto world = volkGetLoadedEcsWorld();
      if(!world->any_of<TagDataChanged>(_id)) {
            world->emplace<TagDataChanged>(_id);
      }
}

void Texture::BarrierLayout(VkCommandBuffer cmd, KernelType new_kernel_type, ResourceUsage new_usage) {

      if(new_kernel_type == _currentKernelType && new_usage == _currentUsage) {
            return;
      }

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

      if (new_kernel_type == KernelType::COMPUTE) {
            switch (new_usage) {
                  case ResourceUsage::SAMPLED:
                        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                        barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
                  break;
                  case ResourceUsage::WRITE_TEXTURE:
                        barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
                        barrier.dstAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
                  break;
                  case ResourceUsage::READ_TEXTURE:
                        barrier.newLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
                        barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
                  break;
                  case ResourceUsage::READ_WRITE_TEXTURE:
                        barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
                        barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
                  break;
                  default: {
                              const auto err  = std::format("Texture::BarrierLayout - The Barrier in Texture, In Compute kernel, resource's New Usage can't be \"{}\"\n", GetResourceUsageString(new_usage));
                              MessageManager::Log(MessageType::Error, err);
                              throw std::runtime_error(err);
                  }
            }

            barrier.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;

      } else if (new_kernel_type == KernelType::GRAPHICS) {
            switch (new_usage) {
                  case ResourceUsage::SAMPLED:
                        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                        barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
                        barrier.dstStageMask = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
                  break;
                  case ResourceUsage::WRITE_TEXTURE:
                        barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
                        barrier.dstAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
                        barrier.dstStageMask = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
                  break;
                  case ResourceUsage::READ_TEXTURE:
                        barrier.newLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
                        barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
                        barrier.dstStageMask = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
                  break;
                  case ResourceUsage::READ_WRITE_TEXTURE:
                        barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
                        barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
                        barrier.dstStageMask = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
                  break;
                  case ResourceUsage::RENDER_TARGET:
                        barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                        barrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
                        barrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
                  break;
                  case ResourceUsage::DEPTH_STENCIL:
                        barrier.newLayout = IsTextureFormatDepthOnly() ? VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                        barrier.dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                        barrier.dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;
                  break;
                  default: {
                              const auto err  = std::format("Texture::BarrierLayout - The Barrier in Texture, In Graphics kernel, resource's New Usage can't be \"{}\"\n", GetResourceUsageString(new_usage));
                              MessageManager::Log(MessageType::Error, err);
                              throw std::runtime_error(err);
                  }
            }
      } else if (new_kernel_type == KernelType::NONE) {
            switch (new_usage) {
                  case ResourceUsage::TRANS_DST:
                        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                        barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
                        barrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
                  break;
                  case ResourceUsage::TRANS_SRC:
                        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                        barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
                        barrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
                  break;
                  case ResourceUsage::PRESENT:
                        barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                        barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
                        barrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
                  break;
                  default: {
                              const auto err  = std::format("Texture::BarrierLayout - The Barrier in Texture, Out of Kernel, resource's New Usage can't be \"{}\"\n", GetResourceUsageString(new_usage));
                              MessageManager::Log(MessageType::Error, err);
                              throw std::runtime_error(err);
                  }
            }
      }

      //old
      if (_currentKernelType == KernelType::COMPUTE) {
            switch (_currentUsage) {
                  case ResourceUsage::SAMPLED:
                        barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                        barrier.srcAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
                  break;
                  case ResourceUsage::WRITE_TEXTURE:
                        barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
                        barrier.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
                  break;
                  case ResourceUsage::READ_TEXTURE:
                        barrier.oldLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
                        barrier.srcAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
                  break;
                  case ResourceUsage::READ_WRITE_TEXTURE:
                        barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
                        barrier.srcAccessMask = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
                  break;
                  default: {
                              const auto err  = std::format("Texture::BarrierLayout - The Barrier in Texture, In Compute kernel, resource's Old Usage can't be \"{}\"\n", GetResourceUsageString(new_usage));
                              MessageManager::Log(MessageType::Error, err);
                              throw std::runtime_error(err);
                  }
            }

            barrier.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;

      } else if (_currentKernelType == KernelType::GRAPHICS) {
            switch (_currentUsage) {
                  case ResourceUsage::SAMPLED:
                        barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                        barrier.srcAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
                        barrier.srcStageMask = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
                  break;
                  case ResourceUsage::WRITE_TEXTURE:
                        barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
                        barrier.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
                        barrier.srcStageMask = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
                  break;
                  case ResourceUsage::READ_TEXTURE:
                        barrier.oldLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
                        barrier.srcAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
                        barrier.srcStageMask = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
                  break;
                  case ResourceUsage::READ_WRITE_TEXTURE:
                        barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
                        barrier.srcAccessMask = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
                        barrier.srcStageMask = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
                  break;
                  case ResourceUsage::RENDER_TARGET:
                        barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                        barrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
                        barrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
                  break;
                  case ResourceUsage::DEPTH_STENCIL:
                        barrier.oldLayout = IsTextureFormatDepthOnly() ? VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                        barrier.srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                        barrier.srcStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;
                  break;
                  default: {
                              const auto err  = std::format("Texture::BarrierLayout - The Barrier in Texture, In Graphics kernel, resource's Old Usage can't be \"{}\"\n", GetResourceUsageString(new_usage));
                              MessageManager::Log(MessageType::Error, err);
                              throw std::runtime_error(err);
                  }
            }
      } else if (_currentKernelType == KernelType::NONE) {
            switch (_currentUsage) {
                  case ResourceUsage::TRANS_DST:
                        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                        barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
                        barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
                  break;
                  case ResourceUsage::TRANS_SRC:
                        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                        barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
                        barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
                  break;
                  case ResourceUsage::PRESENT:
                        barrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                        barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
                        barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
                  break;
                  case ResourceUsage::NONE:
                        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                        barrier.srcAccessMask = 0;
                        barrier.srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                  break;
                  default: {
                              const auto err  = std::format("Texture::BarrierLayout - The Barrier in Texture, Out of Kernel, resource's Old Usage can't be \"{}\"\n", GetResourceUsageString(new_usage));
                              MessageManager::Log(MessageType::Error, err);
                              throw std::runtime_error(err);
                  }
            }
      }

      const VkDependencyInfo info{
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &barrier
      };

      vkCmdPipelineBarrier2(cmd, &info);

      _currentKernelType = new_kernel_type;
      _currentUsage = new_usage;
}

void Texture::ReleaseAllViews() const {
      for (const auto view : _views) {
            ContextResourceRecoveryInfo info{
                  .Type = ContextResourceType::IMAGE_VIEW,
                  .Resource1 = (size_t)view
            };
            GfxContext::Get()->RecoveryContextResource(info);
      }
}

void Texture::Clean() {
      ClearViews();

      if (!_isBorrow) {
            DestroyTexture();
      }
}

void Texture::SetBindlessIndex(std::optional<uint32_t> index) {
      _bindlessIndex = index;
}

void Texture::DestroyTexture() {
      ContextResourceRecoveryInfo info {
            .Type = ContextResourceType::IMAGE,
            .Resource1 = (size_t)_image,
            .Resource2 = (size_t)_memory,
            .Resource3 = _bindlessIndex
      };
      GfxContext::Get()->RecoveryContextResource(info);
}

void Texture::Update(VkCommandBuffer cmd) {
      if(_updateCommand) {
            _updateCommand(cmd);
            _updateCommand = nullptr;
      }
}

void Texture::UpdateAll(VkCommandBuffer cmd) {
      auto& world = *volkGetLoadedEcsWorld();
      auto view = world.view<Texture, TagDataChanged>();
      view.each([cmd](entt::entity e, Texture& texture){
            texture.Update(cmd);
      });

      world.remove<TagDataChanged>(view.begin(), view.end());
}


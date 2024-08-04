#include "Texture.h"

#include "../Message.h"
#include "../GfxContext.h"

using namespace LoFi::Internal;
using namespace LoFi::Component::Gfx;

Texture::~Texture() {
      Clean();
}

Texture::Texture() = default;

Texture::Texture(entt::entity id) : _id(id) {}

bool Texture::Init(const VkImageCreateInfo& image_ci, VkImage Image, bool isBorrow, const char* name) {
      _resourceName = name ? name : std::string{};
      _isBorrow = isBorrow;
      _image = Image;
      _imageCI = std::make_unique<VkImageCreateInfo>(image_ci);
      return true;
}

bool Texture::Init(const VkImageCreateInfo& image_ci, const VmaAllocationCreateInfo& alloc_ci, const GfxParamCreateTexture2D& param) {
      _resourceName = param.pResourceName ? param.pResourceName : std::string{};
      _imageCI = std::make_unique<VkImageCreateInfo>(image_ci);
      _memoryCI = std::make_unique<VmaAllocationCreateInfo>(alloc_ci);

      const auto allocator = volkGetLoadedVmaAllocator();
      if (const auto res = vmaCreateImage(allocator, _imageCI.get(), _memoryCI.get(), &_image, &_memory, nullptr); res != VK_SUCCESS) {
            std::string err = std::format("[TextureCreate] Create Texture Failed! Vulkan return {}.", ToStringVkResult(res));
            if (!_resourceName.empty()) err += std::format(" - Name: \"{}\"", _resourceName);
            MessageManager::Log(MessageType::Error, err);
            return false;
      }
      VmaAllocationInfo info{};
      vmaGetAllocationInfo(volkGetLoadedVmaAllocator(), _memory, &info);

      auto str = std::format("[TextureCreate] Create Texture ({}x{}, {}), {}Bytes.",
      _imageCI->extent.width, _imageCI->extent.height, ToStringVkFormat(_imageCI->format), info.size);
      if (!_resourceName.empty()) str += std::format(" - Name: \"{}\"", _resourceName);
      MessageManager::Log(MessageType::Normal, str);
      return true;
}

VkImageView Texture::CreateView(VkImageViewCreateInfo view_ci) {
      view_ci.image = _image;

      VkImageView view{};
      if (const auto res = vkCreateImageView(volkGetLoadedDevice(), &view_ci, nullptr, &view); res != VK_SUCCESS) {
            auto err = std::format("[Texture::CreateView] Create Texture View. Vulkan return {}.", ToStringVkResult(res));
            if (!_resourceName.empty()) err += std::format(" - Name: \"{}\"", _resourceName);
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
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

bool Texture::Resize(uint32_t w, uint32_t h) {
      if (_isBorrow) {
            std::string err = std::format("[Texture::Resize] Failed to (Resize)Recreate New Texture! Because this is a borrowed teture.");
            if (!_resourceName.empty()) err += std::format(" - Name: \"{}\"", _resourceName);
            MessageManager::Log(MessageType::Warning, err);
            return false;
      }

      if (_imageCI->extent.width == w && _imageCI->extent.height == h) return false;

      const auto allocator = volkGetLoadedVmaAllocator();

      uint32_t back_up_w = _imageCI->extent.width;
      uint32_t back_up_h = _imageCI->extent.height;

      _imageCI->extent.width = w;
      _imageCI->extent.width = h;

      VkImage new_image{};
      if (const auto res = vmaCreateImage(allocator, _imageCI.get(), _memoryCI.get(), &new_image, &_memory, nullptr); res != VK_SUCCESS) {
            std::string err = std::format("[Texture::Resize] Failed to (Resize from {}x{} to {}x{})Recreate New Texture! Vulkan return {}, Fallbacked.", back_up_w, back_up_h, w, h,
            ToStringVkResult(res));
            if (!_resourceName.empty()) err += std::format(" - Name: \"{}\"", _resourceName);
            MessageManager::Log(MessageType::Error, err);
            _imageCI->extent.width = back_up_w;
            _imageCI->extent.width = back_up_h;
            return false;
      }

      //remove all views
      //remove images
      ReleaseAllViews();
      DestroyTexture();

      _image = new_image;

      for (auto& view_ci : _viewCIs) {
            view_ci.image = _image;
            CreateView(view_ci);
      }

      return true;
}

void Texture::SetData(const void* data, size_t size) {
      if (_isBorrow) {
            std::string err = std::format("[Texture::SetData] Failed to SetData Texture! Because this is a borrowed teture.");
            if (!_resourceName.empty()) err += std::format(" - Name: \"{}\"", _resourceName);
            MessageManager::Log(MessageType::Warning, err);
            return;
      }

      VmaAllocationInfo info{};
      vmaGetAllocationInfo(volkGetLoadedVmaAllocator(), _memory, &info);

      if (info.size < size) {
            auto err = std::format("[Texture::SetData] Failed to Set Texture Data! Size Mismatch, Current Size: {}, New Data Size: {}.", info.size, size);
            if (!_resourceName.empty()) err += std::format(" - Name: \"{}\"", _resourceName);
            MessageManager::Log(MessageType::Error, err);
            return;
      }

      if (_intermediateBuffer == nullptr) {
            // create a upload buffer
            auto str = std::format(R"([Texture::SetData] Try UpLoad To Device Texture. Create an Intermediate Buffer {} Bytes)", info.size);
            if (!_resourceName.empty()) str += std::format(" - Name: \"{}\"", _resourceName);
            MessageManager::Log(MessageType::Normal, str);

            _intermediateBuffer = std::make_unique<Buffer>();

            bool success = _intermediateBuffer->Init("Texture Upload Intermediate", VkBufferCreateInfo{
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

            if (!success) return;
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
            this->BarrierLayout(cmd, GfxEnumKernelType::OUT_OF_KERNEL, GfxEnumResourceUsage::TRANS_DST);
            _intermediateBuffer->BarrierLayout(cmd, GfxEnumKernelType::OUT_OF_KERNEL, GfxEnumResourceUsage::TRANS_SRC);
            vkCmdCopyBufferToImage(cmd, imm_buffer, _image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &buffer_copyto_image);
      };

      if (!_bNeedUpdate) {
            LoFi::GfxContext::Get()->EnqueueTextureUpdate(GetHandle());
            _bNeedUpdate = true;
      }
}

void Texture::BarrierLayout(VkCommandBuffer cmd, GfxEnumKernelType new_kernel_type, GfxEnumResourceUsage new_usage) {
      if (new_kernel_type == _currentKernelType && new_usage == _currentUsage) {
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

      if (new_kernel_type == GfxEnumKernelType::COMPUTE) {
            switch (new_usage) {
                  case GfxEnumResourceUsage::SAMPLED:
                        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                        barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
                        break;
                  case GfxEnumResourceUsage::WRITE_TEXTURE:
                        barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
                        barrier.dstAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
                        break;
                  case GfxEnumResourceUsage::READ_TEXTURE:
                        barrier.newLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
                        barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
                        break;
                  case GfxEnumResourceUsage::READ_WRITE_TEXTURE:
                        barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
                        barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
                        break;
                  default:
                        {
                              auto err = std::format("[Texture::BarrierLayout] The Barrier in Texture, In Compute kernel, New Usage can't be \"{}\".", ToStringResourceUsage(new_usage));
                              if (!_resourceName.empty()) err += std::format(" - Name: \"{}\"", _resourceName);
                              MessageManager::Log(MessageType::Error, err);
                              throw std::runtime_error(err);
                        }
            }

            barrier.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
      } else if (new_kernel_type == GfxEnumKernelType::GRAPHICS) {
            switch (new_usage) {
                  case GfxEnumResourceUsage::SAMPLED:
                        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                        barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
                        barrier.dstStageMask = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
                        break;
                  case GfxEnumResourceUsage::WRITE_TEXTURE:
                        barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
                        barrier.dstAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
                        barrier.dstStageMask = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
                        break;
                  case GfxEnumResourceUsage::READ_TEXTURE:
                        barrier.newLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
                        barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
                        barrier.dstStageMask = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
                        break;
                  case GfxEnumResourceUsage::READ_WRITE_TEXTURE:
                        barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
                        barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
                        barrier.dstStageMask = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
                        break;
                  case GfxEnumResourceUsage::RENDER_TARGET:
                        barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                        barrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
                        barrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
                        break;
                  case GfxEnumResourceUsage::DEPTH_STENCIL:
                        barrier.newLayout = IsTextureFormatDepthOnly() ? VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                        barrier.dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                        barrier.dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;
                        break;
                  default:
                        {
                              auto err = std::format("[Texture::BarrierLayout] The Barrier in Texture, In Graphics kernel, New Usage can't be \"{}\".", ToStringResourceUsage(new_usage));
                              if (!_resourceName.empty()) err += std::format(" - Name: \"{}\"", _resourceName);
                              MessageManager::Log(MessageType::Error, err);
                              throw std::runtime_error(err);
                        }
            }
      } else if (new_kernel_type == GfxEnumKernelType::OUT_OF_KERNEL) {
            switch (new_usage) {
                  case GfxEnumResourceUsage::TRANS_DST:
                        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                        barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
                        barrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
                        break;
                  case GfxEnumResourceUsage::TRANS_SRC:
                        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                        barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
                        barrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
                        break;
                  case GfxEnumResourceUsage::PRESENT:
                        barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                        barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
                        barrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
                        break;
                  default:
                        {
                              auto err = std::format("[Texture::BarrierLayout] The Barrier in Texture, Out of Kernel, New Usage can't be \"{}\".", ToStringResourceUsage(new_usage));
                              if (!_resourceName.empty()) err += std::format(" - Name: \"{}\"", _resourceName);
                              MessageManager::Log(MessageType::Error, err);
                              throw std::runtime_error(err);
                        }
            }
      }

      //old
      if (_currentKernelType == GfxEnumKernelType::COMPUTE) {
            switch (_currentUsage) {
                  case GfxEnumResourceUsage::SAMPLED:
                        barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                        barrier.srcAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
                        break;
                  case GfxEnumResourceUsage::WRITE_TEXTURE:
                        barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
                        barrier.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
                        break;
                  case GfxEnumResourceUsage::READ_TEXTURE:
                        barrier.oldLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
                        barrier.srcAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
                        break;
                  case GfxEnumResourceUsage::READ_WRITE_TEXTURE:
                        barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
                        barrier.srcAccessMask = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
                        break;
                  default:
                        {
                              auto err = std::format("[Texture::BarrierLayout] The Barrier in Texture, In Compute kernel, Old Usage can't be \"{}\".", ToStringResourceUsage(new_usage));
                              if (!_resourceName.empty()) err += std::format(" - Name: \"{}\"", _resourceName);
                              MessageManager::Log(MessageType::Error, err);
                              throw std::runtime_error(err);
                        }
            }

            barrier.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
      } else if (_currentKernelType == GfxEnumKernelType::GRAPHICS) {
            switch (_currentUsage) {
                  case GfxEnumResourceUsage::SAMPLED:
                        barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                        barrier.srcAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
                        barrier.srcStageMask = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
                        break;
                  case GfxEnumResourceUsage::WRITE_TEXTURE:
                        barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
                        barrier.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
                        barrier.srcStageMask = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
                        break;
                  case GfxEnumResourceUsage::READ_TEXTURE:
                        barrier.oldLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
                        barrier.srcAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
                        barrier.srcStageMask = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
                        break;
                  case GfxEnumResourceUsage::READ_WRITE_TEXTURE:
                        barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
                        barrier.srcAccessMask = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
                        barrier.srcStageMask = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
                        break;
                  case GfxEnumResourceUsage::RENDER_TARGET:
                        barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                        barrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
                        barrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
                        break;
                  case GfxEnumResourceUsage::DEPTH_STENCIL:
                        barrier.oldLayout = IsTextureFormatDepthOnly() ? VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                        barrier.srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                        barrier.srcStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;
                        break;
                  default:
                        {
                              auto err = std::format("[Texture::BarrierLayout] The Barrier in Texture, In Graphics kernel, Old Usage can't be \"{}\".", ToStringResourceUsage(new_usage));
                              if (!_resourceName.empty()) err += std::format(" - Name: \"{}\"", _resourceName);
                              MessageManager::Log(MessageType::Error, err);
                              throw std::runtime_error(err);
                        }
            }
      } else if (_currentKernelType == GfxEnumKernelType::OUT_OF_KERNEL) {
            switch (_currentUsage) {
                  case GfxEnumResourceUsage::TRANS_DST:
                        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                        barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
                        barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
                        break;
                  case GfxEnumResourceUsage::TRANS_SRC:
                        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                        barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
                        barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
                        break;
                  case GfxEnumResourceUsage::PRESENT:
                        barrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                        barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
                        barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
                        break;
                  case GfxEnumResourceUsage::UNKNOWN_RESOURCE_USAGE:
                        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                        barrier.srcAccessMask = 0;
                        barrier.srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                        break;
                  default:
                        {
                              auto err = std::format("[Texture::BarrierLayout] The Barrier in Texture, Out of Kernel, Old Usage can't be \"{}\".", ToStringResourceUsage(new_usage));
                              if (!_resourceName.empty()) err += std::format(" - Name: \"{}\"", _resourceName);
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

void Texture::SetLayout(GfxEnumKernelType new_kernel_type, GfxEnumResourceUsage new_usage) {
      _currentUsage = new_usage;
      _currentKernelType = new_kernel_type;
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
      if(_isBorrow) return;
      ContextResourceRecoveryInfo info{
            .Type = ContextResourceType::IMAGE,
            .Resource1 = (size_t)_image,
            .Resource2 = (size_t)_memory,
            .Resource3 = _bindlessIndex
      };
      GfxContext::Get()->RecoveryContextResource(info);
}

void Texture::Update(VkCommandBuffer cmd) {
      if (_updateCommand) {
            _updateCommand(cmd);
            _bNeedUpdate = false;
      }
}

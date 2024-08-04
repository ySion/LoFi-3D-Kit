//
// Created by Arzuo on 2024/8/3.
//

#include "RenderNode.h"
#include "GfxContext.h"
#include "Message.h"

#include "GfxComponents/Texture.h"
#include "GfxComponents/Buffer.h"
#include "GfxComponents/Buffer3F.h"
#include "GfxComponents/Kernel.h"
#include "GfxComponents/Swapchain.h"


using namespace LoFi;
using namespace LoFi::Internal;

RenderNodeFrameCommand::RenderNodeFrameCommand(const std::string& name) {
      _nodeName = name;
      VkCommandPoolCreateInfo command_pool_ci{};
      command_pool_ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
      command_pool_ci.queueFamilyIndex = 0;
      command_pool_ci.flags = 0;

      if (const auto res = vkCreateCommandPool(volkGetLoadedDevice(), &command_pool_ci, nullptr, &_cmdPool); res != VK_SUCCESS) {
            std::string err = std::format("[RenderNodeFrameCommand::RenderNodeFrameCommand] vkCreateCommandPool Failed , reutrn {},", ToStringVkResult(res));
            err += std::format(" - Name: \"{}\"", name);
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      VkCommandBufferAllocateInfo second_command_buffer_ai{};
      second_command_buffer_ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
      second_command_buffer_ai.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
      second_command_buffer_ai.commandPool = _cmdPool;
      second_command_buffer_ai.commandBufferCount = 16;

      _secondaryCmdBufsFree.resize(16);

      if (const auto res = vkAllocateCommandBuffers(volkGetLoadedDevice(), &second_command_buffer_ai, _secondaryCmdBufsFree.data());res!= VK_SUCCESS) {
            std::string err = std::format("[RenderNodeFrameCommand::RenderNodeFrameCommand] vkAllocateCommandBuffers Failed, return {}.", ToStringVkResult(res));
            err += std::format(" - Name: \"{}\"", name);
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      std::string msg = std::format("[RenderNodeFrameCommand::RenderNodeFrameCommand] Emplace CommandPool. Success. - {}.", _nodeName);
      MessageManager::Log(MessageType::Normal, msg);
}

RenderNodeFrameCommand::~RenderNodeFrameCommand() {
      vkFreeCommandBuffers(volkGetLoadedDevice(), _cmdPool, static_cast<uint32_t>(_secondaryCmdBufsFree.size()), _secondaryCmdBufsFree.data());
      vkDestroyCommandPool(volkGetLoadedDevice(), _cmdPool, nullptr);
}

void RenderNodeFrameCommand::BeginSecondaryCommandBuffer() {
      if (_secondaryCmdBufsFree.size() < 2) {
            ExpandSecondaryCommandBuffer();
      }

      _current = _secondaryCmdBufsFree.back();
      _secondaryCmdBufsFree.pop_back();
      _secondaryCmdBufsUsed.push_back(_current);

      _prev = _secondaryCmdBufsFree.back();
      _secondaryCmdBufsFree.pop_back();
      _secondaryCmdBufsUsed.push_back(_prev);

      constexpr VkCommandBufferInheritanceInfo inheritance_info{.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO};

      VkCommandBufferBeginInfo info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = nullptr,
            .flags = 0,
            .pInheritanceInfo = &inheritance_info
      };

      vkBeginCommandBuffer(_current, &info);
      vkBeginCommandBuffer(_prev, &info);
}

void RenderNodeFrameCommand::EndSecondaryCommandBuffer() const {
      vkEndCommandBuffer(_current);
      vkEndCommandBuffer(_prev);
}

void RenderNodeFrameCommand::Reset() {
      vkResetCommandPool(volkGetLoadedDevice(), _cmdPool, 0);

      for (auto used_cmd_buf : _secondaryCmdBufsUsed) {
            _secondaryCmdBufsFree.push_back(used_cmd_buf);
      }

      _secondaryCmdBufsUsed.clear();
}

void RenderNodeFrameCommand::ExpandSecondaryCommandBuffer() {
      VkCommandBufferAllocateInfo second_command_buffer_ai{};
      second_command_buffer_ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
      second_command_buffer_ai.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
      second_command_buffer_ai.commandPool = _cmdPool;
      second_command_buffer_ai.commandBufferCount = 16;

      std::vector<VkCommandBuffer> new_buffers(16); //expand 16

      if (const auto res = vkAllocateCommandBuffers(volkGetLoadedDevice(), &second_command_buffer_ai, new_buffers.data()); res != VK_SUCCESS) {
            auto err = std::format("[RenderNodeFrameCommand::ExpandSecondaryCommandBuffer] vkAllocateCommandBuffers Failed, return {}.", ToStringVkResult(res));
            err += std::format(" - Name: \"{}\"", _nodeName);
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      _secondaryCmdBufsFree.reserve(_secondaryCmdBufsFree.size() + 16);
      for (auto& buffer : new_buffers) {
            _secondaryCmdBufsFree.push_back(buffer);
      }
}

RenderNode::RenderNode(entt::entity id, const std::string& name) : _id(id), _nodeName(name) {
      for(int i = 0; i < 3; i++) {
            std::string str = std::format("RenderNode_CommandBuffer_Frame_{}", i);
            _frameCommand[i] = std::make_unique<RenderNodeFrameCommand>(str.c_str());
      }
}

RenderNode::~RenderNode() {
      _frameCommand[0].reset();
      _frameCommand[1].reset();
      _frameCommand[2].reset();
}

void RenderNode::WaitNodes(const GfxInfoRenderNodeWait& param) {
      _waitNodes.clear();
      for(size_t i = 0; i < param.countNamesRenderNodeWaitFor; i++) {
            const char* item = param.pNamesRenderNodeWaitFor[i];
            _waitNodes.emplace_back(item);
      }
}

void RenderNode::CmdBeginComputePass() {
      if (_currentPassType != GfxEnumKernelType::OUT_OF_KERNEL) {
            std::string err = "[RenderNode::CmdBeginComputePass] Already in a pass, Please end it first, Begin Compute Pass Failed.";
            err += std::format(" - Node: \"{}\"", _nodeName);
            MessageManager::Log(MessageType::Error, err);
            return;
      }
      _currentPassType = GfxEnumKernelType::COMPUTE;
      BeginSecondaryCommandBuffer();
}

void RenderNode::CmdEndComputePass() {
      if (_currentPassType != GfxEnumKernelType::COMPUTE) {
            std::string err = "[RenderNode::CmdEndComputePass] Not in a compute pass, create compute pass failed.";
            err += std::format(" - Node: \"{}\"", _nodeName);
            MessageManager::Log(MessageType::Error, err);
            return;
      }

      _currentKernel = {};
      _currentPassType = GfxEnumKernelType::OUT_OF_KERNEL;
     EndSecondaryCommandBuffer();
}

void RenderNode::CmdComputeDispatch(uint32_t x, uint32_t y, uint32_t z) const {
      if (_currentPassType != GfxEnumKernelType::COMPUTE) {
            std::string err = "[RenderNode::CmdComputeDispatch] Not in a compute pass, create compute pass failed.";
            err += std::format(" - Node: \"{}\"", _nodeName);
            MessageManager::Log(MessageType::Error, err);
            return;
      }
      vkCmdDispatch(_current, x, y, z);
}


void RenderNode::CmdBeginRenderPass(const GfxParamBeginRenderPass& param) {
      if (_currentPassType != GfxEnumKernelType::OUT_OF_KERNEL) {
            std::string err = "[RenderNode::CmdBeginRenderPass] Already in a pass, Please end it first, Begin Render Pass Failed.";
            err += std::format(" - Node: \"{}\"", _nodeName);
            MessageManager::Log(MessageType::Error, err);
            return;
      }
      _currentPassType = GfxEnumKernelType::GRAPHICS;
      BeginSecondaryCommandBuffer();

      if (param.countAttachments == 0 || param.pAttachments == nullptr) {
            std::string err = "[RenderNode::CmdBeginRenderPass] GfxParamBeginRenderPass has empty arguments, Create render pass failed.";
            err += std::format(" - Node: \"{}\"", _nodeName);
            MessageManager::Log(MessageType::Error, err);
            return;
      }

      _frameRenderingRenderArea = {};

      VkRenderingInfo render_info = {
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
            .pNext = nullptr,
            .flags = 0,
            .renderArea = {},
            .layerCount = 1,
            .viewMask = 0,
            .colorAttachmentCount = 0,
            .pColorAttachments = nullptr,
            .pDepthAttachment = nullptr,
            .pStencilAttachment = nullptr
      };

      std::vector<VkRenderingAttachmentInfo> _frameRenderingColorAttachments{};
      VkRenderingAttachmentInfo _frameRenderingDepthStencilAttachment{};
      VkRenderingAttachmentInfo _frameRenderingDepthAttachment{};

      _frameRenderingRenderArea = {};
      for(size_t i = 0; i < param.countAttachments; i++){
            const GfxInfoRenderPassaAttachment& info = param.pAttachments[i];

            Component::Gfx::Texture* texture;
            if (info.TextureHandle.Type == GfxEnumResourceType::Texture2D ) {
                  texture = GfxContext::Get()->ResourceFetch<Component::Gfx::Texture>(std::bit_cast<ResourceHandle>(info.TextureHandle));
                  if (!texture) {
                        auto err = std::format("[RenderNode::CmdBeginRenderPass] Invalid Texture2D handle, index at {}.", i);
                        err += std::format(" - Node: \"{}\"", _nodeName);
                        MessageManager::Log(MessageType::Error, err);
                        return;
                  }
            } else if(info.TextureHandle.Type == GfxEnumResourceType::SwapChain) {
                  auto sp = GfxContext::Get()->ResourceFetch<Component::Gfx::Swapchain>(std::bit_cast<ResourceHandle>(info.TextureHandle));
                  if (!sp) {
                        auto err = std::format("[RenderNode::CmdBeginRenderPass] Invalid Swapchain handle, index at {}.", i);
                        err += std::format(" - Node: \"{}\"", _nodeName);
                        MessageManager::Log(MessageType::Error, err);
                        return;
                  }
                  texture = sp->GetCurrentRenderTarget();
            } else {
                  auto err = std::format("[RenderNode::CmdBeginRenderPass] Invalid Resource Type, Need a texture2D or SwapChain, but got {}, index at {}.", ToStringResourceType(info.TextureHandle.Type), i);
                  err += std::format(" - Node: \"{}\"", _nodeName);
                  MessageManager::Log(MessageType::Error, err);
                  return;
            }

            uint32_t view_index = info.ViewIndex;
            bool clear = info.ClearBeforeRendering;

            auto extent = texture->GetExtent();
            if (_frameRenderingRenderArea.extent.width == 0 || _frameRenderingRenderArea.extent.height == 0) {
                  _frameRenderingRenderArea = {0, 0, extent.width, extent.height};
            } else {
                  if (extent.width != _frameRenderingRenderArea.extent.width || extent.height != _frameRenderingRenderArea.extent.height) {
                        auto err = std::format("FrameGraph::BeginPass - Texture size mismatch, expected {}x{}, got {}x{}, index at{}.", _frameRenderingRenderArea.extent.width,
                        _frameRenderingRenderArea.extent.height, extent.width, extent.height, i);
                        err += std::format(" - Node: \"{}\"", _nodeName);
                        MessageManager::Log(MessageType::Error, err);
                        return;
                  }
            }

            if (texture->IsTextureFormatColor()) {
                  // RenderTarget:
                  BarrierTexture(texture, GfxEnumKernelType::GRAPHICS, GfxEnumResourceUsage::RENDER_TARGET);
                  VkRenderingAttachmentInfo render_attachment_info {
                        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                        .imageView = texture->GetView(view_index),
                        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                        .loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
                        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                        .clearValue = {.color = {info.ClearColorR, info.ClearColorG, info.ClearColorB, info.ClearColorA}}
                  };

                  _frameRenderingColorAttachments.push_back(render_attachment_info);
                  render_info.colorAttachmentCount = _frameRenderingColorAttachments.size();
                  render_info.pColorAttachments = _frameRenderingColorAttachments.data();
            } else if (texture->IsTextureFormatDepthOnly()) {
                  // Depth:
                  BarrierTexture(texture, GfxEnumKernelType::GRAPHICS, GfxEnumResourceUsage::DEPTH_STENCIL);
                  _frameRenderingDepthAttachment = VkRenderingAttachmentInfo {
                        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                        .imageView = texture->GetView(view_index),
                        .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                        .loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
                        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                        .clearValue = {.depthStencil = {1.0f, 0}}
                  };

                  render_info.pDepthAttachment = &_frameRenderingDepthAttachment;
                  render_info.pStencilAttachment = nullptr;
            } else if (texture->IsTextureFormatDepthStencilOnly()) {
                  //Depth Stencil
                  BarrierTexture(texture, GfxEnumKernelType::GRAPHICS, GfxEnumResourceUsage::DEPTH_STENCIL);
                  _frameRenderingDepthStencilAttachment = VkRenderingAttachmentInfo{
                        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                        .imageView = texture->GetView(view_index),
                        .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                        .loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
                        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                        .clearValue = {.depthStencil = {1.0f, 0}}
                  };

                  render_info.pDepthAttachment = &_frameRenderingDepthStencilAttachment;
                  render_info.pStencilAttachment = &_frameRenderingDepthStencilAttachment;
            } else {
                  auto err = std::format("[RenderNode::CmdBeginRenderPass] Invalid texture format, format = {}, create render pass failed, index at {}.", ToStringVkFormat(texture->GetFormat()), i);
                  err += std::format(" - Node: \"{}\"", _nodeName);
                  MessageManager::Log(MessageType::Error, err);
                  return;
            }
      }

      render_info.renderArea = _frameRenderingRenderArea;
      vkCmdBeginRenderingKHR(_current, &render_info);
}

void RenderNode::CmdEndRenderPass() {
      if (_currentPassType != GfxEnumKernelType::GRAPHICS) {
            std::string err = "[RenderNode::CmdEndRenderPass] Not in a RenderPass, please use CmdBeginRenderPass first.";
            err += std::format(" - Node: \"{}\"", _nodeName);
            MessageManager::Log(MessageType::Error, err);
            return;
      }
      vkCmdEndRendering(_current);
      EndSecondaryCommandBuffer();
      _currentKernel = {};
      _currentPassType = GfxEnumKernelType::OUT_OF_KERNEL;
}

void RenderNode::CmdBindKernel(ResourceHandle kernel) {
      if (_currentKernel.RHandle == kernel.RHandle) return;

      if (kernel.Type != GfxEnumResourceType::Kernel) {
            auto err = std::format("[RenderNode::CmdBindKernel] Invalid Resource Type, Need a Kernel, but got {}.", ToStringResourceType(kernel.Type));
            err += std::format(" - Node: \"{}\"", _nodeName);
            MessageManager::Log(MessageType::Error, err);
            return;
      }

      auto* kernel_ptr = GfxContext::Get()->ResourceFetch<Component::Gfx::Kernel>(kernel);
      if (!kernel_ptr) {
            auto err = std::format("[RenderNode::CmdBindKernel] Invalid Kernel handle.");
            err += std::format(" - Node: \"{}\"", _nodeName);
            MessageManager::Log(MessageType::Error, err);
            return;
      }

      if (kernel_ptr->IsComputeKernel()) {
            if (_currentPassType != GfxEnumKernelType::COMPUTE) {
                  std::string err = "[RenderNode::CmdBindKernel] Compute kernel must be used in Compute Pass.";
                  err += std::format(" - Node: \"{}\"", _nodeName);
                  MessageManager::Log(MessageType::Error, err);
                  return;
            }
            vkCmdBindPipeline(_current, VK_PIPELINE_BIND_POINT_COMPUTE, kernel_ptr->GetPipeline());
            vkCmdBindDescriptorSets(_current, VK_PIPELINE_BIND_POINT_COMPUTE, kernel_ptr->GetPipelineLayout(),
            0, 1, &GfxContext::Get()->_bindlessDescriptorSet, 0, nullptr);
            kernel_ptr->CmdPushConstants(_current);
      } else if (kernel_ptr->IsGraphicsKernel()) {
            if (_currentPassType != GfxEnumKernelType::GRAPHICS) {
                  std::string err = "[RenderNode::CmdBindKernel] Graphics kernel must be used in Render Pass.";
                  err += std::format(" - Node: \"{}\"", _nodeName);
                  MessageManager::Log(MessageType::Error, err);
                  return;
            }
            vkCmdBindPipeline(_current, VK_PIPELINE_BIND_POINT_GRAPHICS, kernel_ptr->GetPipeline());
            vkCmdBindDescriptorSets(_current, VK_PIPELINE_BIND_POINT_GRAPHICS, kernel_ptr->GetPipelineLayout(), 0, 1, &GfxContext::Get()->_bindlessDescriptorSet, 0, nullptr);
            const auto viewport = VkViewport{0, (float)_frameRenderingRenderArea.extent.height, (float)_frameRenderingRenderArea.extent.width, -(float)_frameRenderingRenderArea.extent.height, 0, 1};
            vkCmdSetViewport(_current, 0, 1, &viewport);
            const auto scissor = VkRect2D{0, 0, _frameRenderingRenderArea.extent.width, _frameRenderingRenderArea.extent.height};
            vkCmdSetScissor(_current, 0, 1, &scissor);
            kernel_ptr->CmdPushConstants(_current);
      } else {
            std::string err = "[RenderNode::CmdBindKernel] this kernel is not a graphics kernel or compute kernel.";
            err += std::format(" - Node: \"{}\"", _nodeName);
            MessageManager::Log(MessageType::Error, err);
            return;
      }

      _currentKernel = kernel;
}

void RenderNode::CmdBindVertexBuffer(ResourceHandle vertex_bufer, uint32_t first_binding, uint32_t binding_count, size_t offset) {
      if (vertex_bufer.Type == GfxEnumResourceType::Buffer) {
            auto* buf = GfxContext::Get()->ResourceFetch<Component::Gfx::Buffer>(vertex_bufer);
            if (!buf) {
                  auto err = std::format("[FrameGraph::BindVertexBuffer] Invalid Buffer handle.");
                  err += std::format(" - Node: \"{}\"", _nodeName);
                  MessageManager::Log(MessageType::Error, err);
                  return;

            }
            BarrierBuffer(buf, GfxEnumKernelType::GRAPHICS, GfxEnumResourceUsage::VERTEX_BUFFER);
            vkCmdBindVertexBuffers(_current, first_binding, binding_count, buf->GetBufferPtr(), &offset);
      } else if (vertex_bufer.Type == GfxEnumResourceType::Buffer3F) {
            auto* buf = GfxContext::Get()->ResourceFetch<Component::Gfx::Buffer3F>(vertex_bufer);
            if (!buf) {
                  auto err = std::format("[FrameGraph::BindVertexBuffer] Invalid Buffer3F handle.");
                  err += std::format(" - Node: \"{}\"", _nodeName);
                  MessageManager::Log(MessageType::Error, err);
                  return;
            }
            BarrierBuffer(buf->GetBufferObject(), GfxEnumKernelType::GRAPHICS, GfxEnumResourceUsage::VERTEX_BUFFER);
            vkCmdBindVertexBuffers(_current, first_binding, binding_count, buf->GetBufferPtr(), &offset);
      } else {
            auto err = std::format("[FrameGraph::BindVertexBuffer] Invalid Resource Type, Need a Buffer or Buffer3F, but got {}.", ToStringResourceType(vertex_bufer.Type));
            err += std::format(" - Node: \"{}\"", _nodeName);
            MessageManager::Log(MessageType::Error, err);
            return;
      }
}

void RenderNode::CmdBindIndexBuffer(ResourceHandle index_buffer, size_t offset) {
      if (index_buffer.Type == GfxEnumResourceType::Buffer) {
            auto* buf = GfxContext::Get()->ResourceFetch<Component::Gfx::Buffer>(index_buffer);
            if (!buf) {
                  auto err = std::format("[RenderNode::CmdBindIndexBuffer] Invalid Buffer handle.");
                  err += std::format(" - Node: \"{}\"", _nodeName);
                  MessageManager::Log(MessageType::Error, err);
                  return;
            }
            BarrierBuffer(buf, GfxEnumKernelType::GRAPHICS, GfxEnumResourceUsage::INDEX_BUFFER);
            vkCmdBindIndexBuffer(_current, buf->GetBuffer(), offset, VK_INDEX_TYPE_UINT32);
      } else if (index_buffer.Type == GfxEnumResourceType::Buffer3F) {
            const auto* buf = GfxContext::Get()->ResourceFetch<Component::Gfx::Buffer3F>(index_buffer);
            if (!buf) {
                  auto err = std::format("[RenderNode::CmdBindIndexBuffer] Invalid Buffer handle.");
                  err += std::format(" - Node: \"{}\"", _nodeName);
                  MessageManager::Log(MessageType::Error, err);
                  return;
            }
            BarrierBuffer(buf->GetBufferObject(), GfxEnumKernelType::GRAPHICS, GfxEnumResourceUsage::INDEX_BUFFER);
            vkCmdBindIndexBuffer(_current, buf->GetBuffer(), offset, VK_INDEX_TYPE_UINT32);
      } else {
            auto err = std::format("[RenderNode::CmdBindIndexBuffer] Invalid Resource Type, Need a Buffer or Buffer3F, but got {}.", ToStringResourceType(index_buffer.Type));
            err += std::format(" - Node: \"{}\"", _nodeName);
            MessageManager::Log(MessageType::Error, err);
            return;
      }
}

void RenderNode::CmdDrawIndexedIndirect(ResourceHandle indirect_buffer, size_t offset, uint32_t draw_count, uint32_t stride) {
      if (indirect_buffer.Type == GfxEnumResourceType::Buffer) {
            auto* buf = GfxContext::Get()->ResourceFetch<Component::Gfx::Buffer>(indirect_buffer);
            if (!buf) {
                  auto err = std::format("[RenderNode::CmdDrawIndexedIndirect] Invalid Buffer handle.");
                  err += std::format(" - Node: \"{}\"", _nodeName);
                  MessageManager::Log(MessageType::Error, err);
                  return;
            }
            BarrierBuffer(buf, GfxEnumKernelType::GRAPHICS, GfxEnumResourceUsage::INDIRECT_BUFFER);
            vkCmdDrawIndexedIndirect(_current, buf->GetBuffer(), offset, draw_count, stride);
      } else if (indirect_buffer.Type == GfxEnumResourceType::Buffer3F) {
            auto* buf = GfxContext::Get()->ResourceFetch<Component::Gfx::Buffer3F>(indirect_buffer);
            if (!buf) {
                  auto err = std::format("[RenderNode::CmdDrawIndexedIndirect] Invalid Buffer handle.");
                  err += std::format(" - Node: \"{}\"", _nodeName);
                  MessageManager::Log(MessageType::Error, err);
                  return;
            }
            BarrierBuffer(buf->GetBufferObject(), GfxEnumKernelType::GRAPHICS, GfxEnumResourceUsage::INDIRECT_BUFFER);
            vkCmdDrawIndexedIndirect(_current, buf->GetBuffer(), offset, draw_count, stride);
      } else {
            auto err = std::format("[RenderNode::CmdDrawIndexedIndirect] Invalid Resource Type, Need a Buffer or Buffer3F, but got {}.", ToStringResourceType(indirect_buffer.Type));
            err += std::format(" - Node: \"{}\"", _nodeName);
            MessageManager::Log(MessageType::Error, err);
            return;
      }
}

void RenderNode::CmdDrawIndex(uint32_t index_count, uint32_t instance_count, uint32_t first_index, int32_t vertex_offset, uint32_t first_instance) const {
      vkCmdDrawIndexed(_current, index_count, instance_count, first_index, vertex_offset, first_instance);
}


void RenderNode::CmdDraw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance) const {
      vkCmdDraw(_current, vertex_count, instance_count, first_vertex, first_instance);
}

void RenderNode::CmdSetViewport(const VkViewport& viewport) const {
      vkCmdSetViewport(_current, 0, 1, &viewport);
}

void RenderNode::CmdSetScissor(VkRect2D scissor) const {
      vkCmdSetScissor(_current, 0, 1, &scissor);
}

void RenderNode::CmdSetViewportAuto(bool invert_y) const {
      if (invert_y) {
            const auto viewport = VkViewport{0, (float)_frameRenderingRenderArea.extent.height, (float)_frameRenderingRenderArea.extent.width, -(float)_frameRenderingRenderArea.extent.height, 0, 1};
            vkCmdSetViewport(_current, 0, 1, &viewport);
      } else {
            const auto viewport = VkViewport{0, 0, (float)_frameRenderingRenderArea.extent.width, (float)_frameRenderingRenderArea.extent.height, 0, 1};
            vkCmdSetViewport(_current, 0, 1, &viewport);
      }
}

void RenderNode::CmdSetScissorAuto() const {
      const auto scissor = VkRect2D{0, 0, _frameRenderingRenderArea.extent.width, _frameRenderingRenderArea.extent.height};
      vkCmdSetScissor(_current, 0, 1, &scissor);
}

void RenderNode::CmdPushConstant(GfxInfoKernelLayout kernel_layout, const void* data, size_t data_size) const {
     if(kernel_layout.Layout == 0) {
           auto err = std::format("[RenderNode::CmdPushConstant] Invalid Kernel Layout.");
           err += std::format(" - Node: \"{}\"", _nodeName);
           MessageManager::Log(MessageType::Error, err);
           return;
     } else {
           uint32_t size = std::min((size_t)kernel_layout.Size, data_size);
           vkCmdPushConstants(_current, std::bit_cast<VkPipelineLayout>(kernel_layout.Layout), VkShaderStageFlagBits::VK_SHADER_STAGE_ALL, kernel_layout.Offset, size, data);
     }
}

void RenderNode::CmdAsSampledTexure(ResourceHandle texture, GfxEnumKernelType which_kernel_use) {
      if (texture.Type == GfxEnumResourceType::Texture2D) {
            auto* ptr = GfxContext::Get()->ResourceFetch<Component::Gfx::Texture>(texture);
            if (!ptr) {
                  auto err = std::format("[RenderNode::CmdAsSampledTexure] Invalid Texture handle.");
                  err += std::format(" - Node: \"{}\"", _nodeName);
                  MessageManager::Log(MessageType::Error, err);
                  return;
            }
            if(which_kernel_use == GfxEnumKernelType::OUT_OF_KERNEL) { // Auto
                  if(_currentPassType == GfxEnumKernelType::OUT_OF_KERNEL) {
                        auto err = std::format("[RenderNode::CmdAsSampledTexure] Not in Any pass, Can't auto detect kernel type, Please use in a pass.");
                        err += std::format(" - Node: \"{}\", Texture: {}", _nodeName, ptr->GetResourceName());
                        MessageManager::Log(MessageType::Error, err);
                        return;
                  } else {
                        BarrierTexture(ptr, _currentPassType, GfxEnumResourceUsage::SAMPLED);
                  }
            } else {
                  BarrierTexture(ptr, which_kernel_use, GfxEnumResourceUsage::SAMPLED);
            }
      } else if(texture.Type == GfxEnumResourceType::SwapChain) {
            const auto sp =  GfxContext::Get()->ResourceFetch<Component::Gfx::Swapchain>(texture);
            if (!sp) {
                  auto err = std::format("[RenderNode::CmdAsSampledTexure] Invalid SwapChain handle.");
                  err += std::format(" - Node: \"{}\"", _nodeName);
                  MessageManager::Log(MessageType::Error, err);
                  return;
            }
            auto* ptr = sp->GetCurrentRenderTarget();
            if(which_kernel_use == GfxEnumKernelType::OUT_OF_KERNEL) { // Auto
                  if(_currentPassType == GfxEnumKernelType::OUT_OF_KERNEL) {
                        auto err = std::format("[RenderNode::CmdAsSampledTexure] Not in Any pass, Can't auto detect kernel type, Please use in a pass.");
                        err += std::format(" - Node: \"{}\", Texture: {}", _nodeName, ptr->GetResourceName());
                        MessageManager::Log(MessageType::Error, err);
                        return;
                  } else {
                        BarrierTexture(ptr, _currentPassType, GfxEnumResourceUsage::SAMPLED);
                  }
            } else {
                  BarrierTexture(ptr, which_kernel_use, GfxEnumResourceUsage::SAMPLED);
            }
      } else {
            auto err = std::format("[RenderNode::CmdAsSampledTexure] Invalid Resource Type, Need a Texture or SwapChain, but got {}.", ToStringResourceType(texture.Type));
            err += std::format(" - Node: \"{}\"", _nodeName);
            MessageManager::Log(MessageType::Error, err);
            return;
      }
}

void RenderNode::CmdAsReadTexure(ResourceHandle texture, GfxEnumKernelType which_kernel_use) {
      if (texture.Type == GfxEnumResourceType::Texture2D) {
            auto* ptr = GfxContext::Get()->ResourceFetch<Component::Gfx::Texture>(texture);
            if (!ptr) {
                  auto err = std::format("[RenderNode::CmdAsReadTexure] Invalid Texture handle.");
                  err += std::format(" - Node: \"{}\"", _nodeName);
                  MessageManager::Log(MessageType::Error, err);
                  return;
            }
            if(which_kernel_use == GfxEnumKernelType::OUT_OF_KERNEL) { // Auto
                  if(_currentPassType == GfxEnumKernelType::OUT_OF_KERNEL) {
                        auto err = std::format("[RenderNode::CmdAsReadTexure] Not in Any pass, Can't auto detect kernel type, Please use in a pass.");
                        err += std::format(" - Node: \"{}\", Texture: {}", _nodeName, ptr->GetResourceName());
                        MessageManager::Log(MessageType::Error, err);
                        return;
                  } else {
                        BarrierTexture(ptr, _currentPassType, GfxEnumResourceUsage::READ_TEXTURE);
                  }
            } else {
                  BarrierTexture(ptr, which_kernel_use, GfxEnumResourceUsage::READ_TEXTURE);
            }
      } else if(texture.Type == GfxEnumResourceType::SwapChain) {
            const auto sp =  GfxContext::Get()->ResourceFetch<Component::Gfx::Swapchain>(texture);
            if (!sp) {
                  auto err = std::format("[RenderNode::CmdAsReadTexure] Invalid SwapChain handle.");
                  err += std::format(" - Node: \"{}\"", _nodeName);
                  MessageManager::Log(MessageType::Error, err);
                  return;
            }
            auto* ptr = sp->GetCurrentRenderTarget();
            if(which_kernel_use == GfxEnumKernelType::OUT_OF_KERNEL) { // Auto
                  if(_currentPassType == GfxEnumKernelType::OUT_OF_KERNEL) {
                        auto err = std::format("[RenderNode::CmdAsReadTexure] Not in Any pass, Can't auto detect kernel type, Please use in a pass.");
                        err += std::format(" - Node: \"{}\", Texture: {}", _nodeName, ptr->GetResourceName());
                        MessageManager::Log(MessageType::Error, err);
                        return;
                  } else {
                        BarrierTexture(ptr, _currentPassType, GfxEnumResourceUsage::READ_TEXTURE);
                  }
            } else {
                  BarrierTexture(ptr, which_kernel_use, GfxEnumResourceUsage::READ_TEXTURE);
            }
      } else {
            auto err = std::format("[RenderNode::CmdAsReadTexure] Invalid Resource Type, Need a Texture, but got {}.", ToStringResourceType(texture.Type));
            err += std::format(" - Node: \"{}\"", _nodeName);
            MessageManager::Log(MessageType::Error, err);
            return;
      }
}

void RenderNode::CmdAsWriteTexture(ResourceHandle texture, GfxEnumKernelType which_kernel_use) {
      if (texture.Type == GfxEnumResourceType::Texture2D) {
            auto* ptr = GfxContext::Get()->ResourceFetch<Component::Gfx::Texture>(texture);
            if (!ptr) {
                  auto err = std::format("[RenderNode::CmdAsWriteTexture] Invalid Texture handle.");
                  err += std::format(" - Node: \"{}\"", _nodeName);
                  MessageManager::Log(MessageType::Error, err);
                  return;
            }
            if(which_kernel_use == GfxEnumKernelType::OUT_OF_KERNEL) { // Auto
                  if(_currentPassType == GfxEnumKernelType::OUT_OF_KERNEL) {
                        auto err = std::format("[RenderNode::CmdAsWriteTexture] Not in Any pass, Can't auto detect kernel type, Please use in a pass.");
                        err += std::format(" - Node: \"{}\", Texture: {}", _nodeName, ptr->GetResourceName());
                        MessageManager::Log(MessageType::Error, err);
                        return;
                  } else {
                        BarrierTexture(ptr, _currentPassType, GfxEnumResourceUsage::WRITE_TEXTURE);
                  }
            } else {
                  BarrierTexture(ptr, which_kernel_use, GfxEnumResourceUsage::WRITE_TEXTURE);
            }
      } else if(texture.Type == GfxEnumResourceType::SwapChain) {
            const auto sp =  GfxContext::Get()->ResourceFetch<Component::Gfx::Swapchain>(texture);
            if (!sp) {
                  auto err = std::format("[RenderNode::CmdAsWriteTexture] Invalid SwapChain handle.");
                  err += std::format(" - Node: \"{}\"", _nodeName);
                  MessageManager::Log(MessageType::Error, err);
                  return;
            }
            auto* ptr = sp->GetCurrentRenderTarget();
            if(which_kernel_use == GfxEnumKernelType::OUT_OF_KERNEL) { // Auto
                  if(_currentPassType == GfxEnumKernelType::OUT_OF_KERNEL) {
                        auto err = std::format("[RenderNode::CmdAsWriteTexture] Not in Any pass, Can't auto detect kernel type, Please use in a pass.");
                        err += std::format(" - Node: \"{}\", Texture: {}", _nodeName, ptr->GetResourceName());
                        MessageManager::Log(MessageType::Error, err);
                        return;
                  } else {
                        BarrierTexture(ptr, _currentPassType, GfxEnumResourceUsage::WRITE_TEXTURE);
                  }
            } else {
                  BarrierTexture(ptr, which_kernel_use, GfxEnumResourceUsage::WRITE_TEXTURE);
            }
      } else {
            auto err = std::format("[RenderNode::CmdAsWriteTexture] Invalid Resource Type, Need a Texture, but got {}.", ToStringResourceType(texture.Type));
            err += std::format(" - Node: \"{}\"", _nodeName);
            MessageManager::Log(MessageType::Error, err);
            return;
      }
}

void RenderNode::CmdAsWriteBuffer(ResourceHandle buffer, GfxEnumKernelType which_kernel_use) {
      if (buffer.Type == GfxEnumResourceType::Buffer) {
            auto* ptr = GfxContext::Get()->ResourceFetch<Component::Gfx::Buffer>(buffer);
            if (!ptr) {
                  auto err = std::format("[RenderNode::CmdAsWriteBuffer] Invalid Buffer handle.");
                  err += std::format(" - Node: \"{}\"", _nodeName);
                  MessageManager::Log(MessageType::Error, err);
                  return;
            }
            if(which_kernel_use == GfxEnumKernelType::OUT_OF_KERNEL) { // Auto
                  if(_currentPassType == GfxEnumKernelType::OUT_OF_KERNEL) {
                        auto err = std::format("[RenderNode::CmdAsWriteBuffer] Not in Any pass, Can't auto detect kernel type, Please use in a pass.");
                        err += std::format(" - Node: \"{}\", Buffer: {}", _nodeName, ptr->GetResourceName());
                        MessageManager::Log(MessageType::Error, err);
                        return;
                  } else {
                        BarrierBuffer(ptr, _currentPassType, GfxEnumResourceUsage::WRITE_BUFFER);
                  }
            } else {
                  BarrierBuffer(ptr, which_kernel_use, GfxEnumResourceUsage::WRITE_BUFFER);
            }
      } else if (buffer.Type == GfxEnumResourceType::Buffer3F) {
            auto* ptr = GfxContext::Get()->ResourceFetch<Component::Gfx::Buffer3F>(buffer);
            if (!ptr) {
                  auto err = std::format("[RenderNode::CmdAsWriteBuffer] Invalid Buffer3F handle.");
                  err += std::format(" - Node: \"{}\"", _nodeName);
                  MessageManager::Log(MessageType::Error, err);
                  return;
            }

            if(which_kernel_use == GfxEnumKernelType::OUT_OF_KERNEL) { // Auto
                  if(_currentPassType == GfxEnumKernelType::OUT_OF_KERNEL) {
                        auto err = std::format("[RenderNode::CmdAsWriteBuffer] Not in Any pass, Can't auto detect kernel type, Please use in a pass.");
                        err += std::format(" - Node: \"{}\", Buffer3F: {}", _nodeName, ptr->GetResourceName());
                        MessageManager::Log(MessageType::Error, err);
                        return;
                  } else {
                        BarrierBuffer(ptr->GetBufferObject(), which_kernel_use, GfxEnumResourceUsage::WRITE_BUFFER);
                  }
            } else {
                  BarrierBuffer(ptr->GetBufferObject(), which_kernel_use, GfxEnumResourceUsage::WRITE_BUFFER);
            }
      } else {
            auto err = std::format("[RenderNode::CmdAsWriteBuffer] Invalid Resource Type, Need a Buffer or Buffer3F, but got {}.", ToStringResourceType(buffer.Type));
            err += std::format(" - Node: \"{}\"", _nodeName);
            MessageManager::Log(MessageType::Error, err);
            return;
      }
}

void RenderNode::CmdAsReadBuffer(ResourceHandle buffer, GfxEnumKernelType which_kernel_use) {
      if (buffer.Type == GfxEnumResourceType::Buffer) {
            auto* ptr = GfxContext::Get()->ResourceFetch<Component::Gfx::Buffer>(buffer);
            if (!ptr) {
                  auto err = std::format("[RenderNode::CmdAsReadBuffer] Invalid Buffer handle.");
                  err += std::format(" - Node: \"{}\"", _nodeName);
                  MessageManager::Log(MessageType::Error, err);
                  return;
            }
            if(which_kernel_use == GfxEnumKernelType::OUT_OF_KERNEL) { // Auto
                  if(_currentPassType == GfxEnumKernelType::OUT_OF_KERNEL) {
                        auto err = std::format("[RenderNode::CmdAsReadBuffer] Not in Any pass, Can't auto detect kernel type, Please use in a pass.");
                        err += std::format(" - Node: \"{}\", Buffer: {}", _nodeName, ptr->GetResourceName());
                        MessageManager::Log(MessageType::Error, err);
                        return;
                  } else {
                        BarrierBuffer(ptr, _currentPassType, GfxEnumResourceUsage::READ_BUFFER);
                  }
            } else {
                  BarrierBuffer(ptr, which_kernel_use, GfxEnumResourceUsage::READ_BUFFER);
            }
      } else if (buffer.Type == GfxEnumResourceType::Buffer3F) {
            auto* ptr = GfxContext::Get()->ResourceFetch<Component::Gfx::Buffer3F>(buffer);
            if (!ptr) {
                  auto err = std::format("[RenderNode::CmdAsReadBuffer] Invalid Buffer3F handle.");
                  err += std::format(" - Node: \"{}\"", _nodeName);
                  MessageManager::Log(MessageType::Error, err);
                  return;
            }

            if(which_kernel_use == GfxEnumKernelType::OUT_OF_KERNEL) { // Auto
                  if(_currentPassType == GfxEnumKernelType::OUT_OF_KERNEL) {
                        auto err = std::format("[RenderNode::CmdAsReadBuffer] Not in Any pass, Can't auto detect kernel type, Please use in a pass.");
                        err += std::format(" - Node: \"{}\", Buffer3F: {}", _nodeName, ptr->GetResourceName());
                        MessageManager::Log(MessageType::Error, err);
                        return;
                  } else {
                        BarrierBuffer(ptr->GetBufferObject(), which_kernel_use, GfxEnumResourceUsage::READ_BUFFER);
                  }
            } else {
                  BarrierBuffer(ptr->GetBufferObject(), which_kernel_use, GfxEnumResourceUsage::READ_BUFFER);
            }
      } else {
            auto err = std::format("[RenderNode::CmdAsReadBuffer] Invalid Resource Type, Need a Buffer or Buffer3F, but got {}.", ToStringResourceType(buffer.Type));
            err += std::format(" - Node: \"{}\"", _nodeName);
            MessageManager::Log(MessageType::Error, err);
            return;
      }
}

void RenderNode::EmitCommands(VkCommandBuffer primary_cmdbuf) const {
      auto& cmds = GetCurrentFrameCommand()->GetSecondaryCommandBuffers();
      if(cmds.empty()) { return; }
      vkCmdExecuteCommands(primary_cmdbuf, cmds.size(), cmds.data());
}


void RenderNode::BarrierTexture(Component::Gfx::Texture* texture, GfxEnumKernelType new_kernel_type, GfxEnumResourceUsage new_usage) {
      if(const auto find = _barrierTableTexture.find(texture); find != _barrierTableTexture.end()) {
            if (find->second.kernel_type == new_kernel_type && find->second.usage == new_usage) {
                  return;
            } else {
                  CmdBarrierTexture(_prev, texture, find->second.kernel_type, find->second.usage, new_kernel_type, new_usage, _nodeName);
                  find->second.kernel_type = new_kernel_type;
                  find->second.usage = new_usage;
            }
      } else {
            _beginBarrierTexture.emplace_back(texture, new_kernel_type, new_usage);
            _barrierTableTexture[texture] = {new_kernel_type, new_usage};
      }
}

void RenderNode::BarrierBuffer(Component::Gfx::Buffer* buffer, GfxEnumKernelType new_kernel_type, GfxEnumResourceUsage new_usage) {
      if(const auto find = _barrierTableBuffer.find(buffer); find != _barrierTableBuffer.end()) {
            if (find->second.kernel_type == new_kernel_type && find->second.usage == new_usage) {
                  return;
            } else {
                  CmdBarrierBuffer(_prev, buffer, find->second.kernel_type, find->second.usage, new_kernel_type, new_usage, _nodeName);
                  find->second.kernel_type = new_kernel_type;
                  find->second.usage = new_usage;
            }
      } else {
            _beginBarrierBuffer.emplace_back(buffer, new_kernel_type, new_usage);
            _barrierTableBuffer[buffer] = {new_kernel_type, new_usage};
      }
}

void RenderNode::PrepareFrame() {
      GetCurrentFrameCommand()->Reset();
      _beginBarrierTexture.clear();
      _beginBarrierBuffer.clear();
      _barrierTableTexture.clear();
      _barrierTableBuffer.clear();
}

void RenderNode::CmdBarrierTexture(VkCommandBuffer buf, Component::Gfx::Texture* texture, GfxEnumKernelType old_kernel_type, GfxEnumResourceUsage old_usage, GfxEnumKernelType new_kernel_type, GfxEnumResourceUsage new_usage, std::string_view _nodeName) {

      VkImageMemoryBarrier2 barrier{};
      barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR;
      barrier.image = texture->GetImage();
      barrier.subresourceRange = {
            .aspectMask = texture->GetViewCI().subresourceRange.aspectMask,
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
                              auto err = std::format("[RenderNode::CmdBarrierTexture] The Barrier in Texture, In Compute kernel, New Usage can't be \"{}\".", ToStringResourceUsage(new_usage));
                              err += std::format(R"( - Name: "{}", Texture: "{}".)", _nodeName , texture->GetResourceName());
                              MessageManager::Log(MessageType::Error, err);
                              return;
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
                        barrier.newLayout = texture->IsTextureFormatDepthOnly() ? VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                        barrier.dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                        barrier.dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;
                        break;
                  default:
                        {
                              auto err = std::format("[RenderNode::CmdBarrierTexture] The Barrier in Texture, In Graphics kernel, New Usage can't be \"{}\".", ToStringResourceUsage(new_usage));
                              err += std::format(R"( - Name: "{}", Texture: "{}".)", _nodeName , texture->GetResourceName());
                              MessageManager::Log(MessageType::Error, err);
                              return;
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
                              auto err = std::format("[RenderNode::CmdBarrierTexture] The Barrier in Texture, Out of Kernel, New Usage can't be \"{}\".", ToStringResourceUsage(new_usage));
                              err += std::format(R"( - Name: "{}", Texture: "{}".)", _nodeName , texture->GetResourceName());
                              MessageManager::Log(MessageType::Error, err);
                              return;
                        }
            }
      }

      //old
      if (old_kernel_type == GfxEnumKernelType::COMPUTE) {
            switch (old_usage) {
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
                              auto err = std::format("[RenderNode::CmdBarrierTexture] The Barrier in Texture, In Compute kernel, Old Usage can't be \"{}\".", ToStringResourceUsage(new_usage));
                              err += std::format(R"( - Name: "{}", Texture: "{}".)", _nodeName , texture->GetResourceName());
                              MessageManager::Log(MessageType::Error, err);
                              return;
                        }
            }

            barrier.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
      } else if (old_kernel_type == GfxEnumKernelType::GRAPHICS) {
            switch (old_usage) {
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
                        barrier.oldLayout = texture->IsTextureFormatDepthOnly() ? VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                        barrier.srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                        barrier.srcStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;
                        break;
                  default:
                        {
                              auto err = std::format("[RenderNode::CmdBarrierTexture] The Barrier in Texture, In Graphics kernel, Old Usage can't be \"{}\".", ToStringResourceUsage(new_usage));
                              err += std::format(R"( - Name: "{}", Texture: "{}".)", _nodeName , texture->GetResourceName());
                              MessageManager::Log(MessageType::Error, err);
                              return;
                        }
            }
      } else if (old_kernel_type == GfxEnumKernelType::OUT_OF_KERNEL) {
            switch (old_usage) {
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
                              auto err = std::format("[RenderNode::CmdBarrierTexture] The Barrier in Texture, Out of Kernel, Old Usage can't be \"{}\".", ToStringResourceUsage(new_usage));
                              err += std::format(R"( - Name: "{}", Texture: "{}".)", _nodeName , texture->GetResourceName());
                              MessageManager::Log(MessageType::Error, err);
                              return;
                        }
            }
      }

      const VkDependencyInfo info{
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &barrier
      };

      vkCmdPipelineBarrier2(buf, &info);
}

void RenderNode::CmdBarrierBuffer(VkCommandBuffer buf, Component::Gfx::Buffer* buffer, GfxEnumKernelType old_kernel_type, GfxEnumResourceUsage old_usage, GfxEnumKernelType new_kernel_type, GfxEnumResourceUsage new_usage, std::string_view _nodeName)
{
      VkBufferMemoryBarrier2 barrier{};
      barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
      barrier.buffer = buffer->GetBuffer();
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
                              auto err = std::format("[RenderNode::CmdBarrierBuffer] The Barrier in Buffer, In Compute kernel, New Usage can't be \"{}\".", ToStringResourceUsage(new_usage));
                              err += std::format(R"( - Name: "{}", Texture: "{}".)", _nodeName , buffer->GetResourceName());
                              MessageManager::Log(MessageType::Error, err);
                              return;
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
                              auto err = std::format("[RenderNode::CmdBarrierBuffer] The Barrier in Buffer, In Compute kernel, New Usage can't be \"{}\".", ToStringResourceUsage(new_usage));
                              err += std::format(R"( - Name: "{}", Buffer: "{}".)", _nodeName , buffer->GetResourceName());
                              MessageManager::Log(MessageType::Error, err);
                              return;
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
                              auto err = std::format("[RenderNode::CmdBarrierBuffer] The Barrier in Buffer, In Compute kernel, New Usage can't be \"{}\".", ToStringResourceUsage(new_usage));
                              err += std::format(R"( - Name: "{}", Buffer: "{}".)", _nodeName , buffer->GetResourceName());
                              MessageManager::Log(MessageType::Error, err);
                              return;
                        }
            }
      }

      //old
      if (old_kernel_type == GfxEnumKernelType::COMPUTE) {
            switch (old_usage) {
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
                              auto err = std::format("[RenderNode::CmdBarrierBuffer] The Barrier in Buffer, In Compute kernel, Old Usage can't be \"{}\".", ToStringResourceUsage(new_usage));
                              err += std::format(R"( - Name: "{}", Buffer: "{}".)", _nodeName , buffer->GetResourceName());
                              MessageManager::Log(MessageType::Error, err);
                              return;
                        }
            }
      } else if (old_kernel_type == GfxEnumKernelType::GRAPHICS) {
            switch (old_usage) {
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
                              auto err = std::format("[RenderNode::CmdBarrierBuffer] The Barrier in Buffer, In Compute kernel, Old Usage can't be \"{}\".", ToStringResourceUsage(new_usage));
                              err += std::format(R"( - Name: "{}", Buffer: "{}".)", _nodeName , buffer->GetResourceName());
                              MessageManager::Log(MessageType::Error, err);
                              return;
                        }
            }
      } else if (old_kernel_type == GfxEnumKernelType::OUT_OF_KERNEL) {
            switch (old_usage) {
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
                              auto err = std::format("[RenderNode::CmdBarrierBuffer] The Barrier in Buffer, In Compute kernel, Old Usage can't be \"{}\".", ToStringResourceUsage(new_usage));
                              err += std::format(R"( - Name: "{}", Buffer: "{}".)", _nodeName , buffer->GetResourceName());
                              MessageManager::Log(MessageType::Error, err);
                              return;
                        }
            }
      }

      const VkDependencyInfo info{
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .bufferMemoryBarrierCount = 1,
            .pBufferMemoryBarriers = &barrier
      };

      vkCmdPipelineBarrier2(buf, &info);
}

void RenderNode::BeginSecondaryCommandBuffer() {
      GetCurrentFrameCommand()->BeginSecondaryCommandBuffer();
      _prev = GetCurrentFrameCommand()->GetCommandBufferPrev();
      _current = GetCurrentFrameCommand()->GetCommandBufferCurr();
}

void RenderNode::EndSecondaryCommandBuffer() {
      _prev = nullptr;
      _current = nullptr;
      GetCurrentFrameCommand()->EndSecondaryCommandBuffer();
}





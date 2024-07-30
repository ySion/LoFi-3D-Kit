//
// Created by Arzuo on 2024/7/8.
//
#include "FrameGraph.h"
#include "Message.h"
#include "GfxContext.h"
#include "GfxComponents/Texture.h"
#include "GfxComponents/Kernel.h"
#include "GfxComponents/PushConstantBuffer.h"

using namespace LoFi::Internal;

LoFi::FrameGraph::~FrameGraph() {
      vkDestroyCommandPool(volkGetLoadedDevice(), _secondaryCmdPool, nullptr);
}

LoFi::FrameGraph::FrameGraph(VkCommandBuffer cmd_buf) : _cmdBuffer(cmd_buf), _world(*volkGetLoadedEcsWorld()) {
      VkCommandPoolCreateInfo command_pool_ci{};
      command_pool_ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
      command_pool_ci.queueFamilyIndex = 0;
      command_pool_ci.flags = 0;

      if (vkCreateCommandPool(volkGetLoadedDevice(), &command_pool_ci, nullptr, &_secondaryCmdPool) != VK_SUCCESS) {
            MessageManager::Log(MessageType::Error, "FrameGraph::FrameGraph - Failed to create command pool");
            throw std::runtime_error("FrameGraph::FrameGraph - Failed to create command pool");
      }

      VkCommandBufferAllocateInfo second_command_buffer_ai{};
      second_command_buffer_ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
      second_command_buffer_ai.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
      second_command_buffer_ai.commandPool = _secondaryCmdPool;
      second_command_buffer_ai.commandBufferCount = 16;

      _secondaryCmdBufsFree.resize(16);

      if (vkAllocateCommandBuffers(volkGetLoadedDevice(), &second_command_buffer_ai, _secondaryCmdBufsFree.data()) != VK_SUCCESS) {
            const auto err = "FrameGraph::ExpandSecondaryCommandBuffer - Failed to allocate secondary command buffers";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }
}

void LoFi::FrameGraph::BeginFrame() {

      //sub cmds
      vkResetCommandPool(volkGetLoadedDevice(), _secondaryCmdPool, 0);

      for (auto used_cmd_buf : _secondaryCmdBufsUsed) {
            _secondaryCmdBufsFree.push_back(used_cmd_buf);
      }

      _secondaryCmdBufsUsed.clear();

      if (vkResetCommandBuffer(_cmdBuffer, 0) != VK_SUCCESS) {
            const auto err = "FrameGraph::Reset Failed to reset command buffer";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      VkCommandBufferBeginInfo begin_info{};
      begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      if (vkBeginCommandBuffer(_cmdBuffer, &begin_info) != VK_SUCCESS) {
            const auto err = "FrameGraph::BeginFrame Failed to begin command buffer";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }
}

void LoFi::FrameGraph::EndFrame() const {
      if (vkEndCommandBuffer(_cmdBuffer) != VK_SUCCESS) {
            const auto err = "FrameGraph::EndFrame Failed to end command buffer";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }
}

void LoFi::FrameGraph::BeginComputePass() {
      if(_passType != NONE) {
            const auto err = "FrameGraph::BeginComputePass - Already in a pass, Please end it first, Begin Compute Pass Failed.";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }
      _passType = COMPUTE;
      BeginSecondaryCommandBuffer();
}

void LoFi::FrameGraph::EndComputePass() {
      if(_passType != COMPUTE) {
            const auto err = "FrameGraph::EndComputePass - Not in a compute pass, create compute pass failed.";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      _currentKernel = entt::null;
      EndSecondaryCommandBuffer();
      _passType = NONE;
}

void LoFi::FrameGraph::ComputeDispatch(uint32_t x, uint32_t y, uint32_t z) const {
      vkCmdDispatch(_current, x, y, z);
}

void LoFi::FrameGraph::BeginRenderPass(const std::vector<RenderPassBeginArgument>& textures) {
      if (_passType != NONE) {
            const auto err = "FrameGraph::BeginPass - Already in a pass, Please end it first, Begin Render Pass Failed.";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }
      _passType = GRAPHICS;
      BeginSecondaryCommandBuffer();

      if (textures.empty()) {
            const auto err = "FrameGraph::BeginRenderPass - Empty textures, create render pass failed.";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
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
      for (const auto& param : textures) {
            entt::entity handle = param.TextureHandle;
            uint32_t view_index = param.ViewIndex;
            bool clear = param.ClearBeforeRendering;
            if (!_world.valid(handle)) {
                  const auto err = std::format("FrameGraph::BeginPass - Invalid texture entity.");
                  MessageManager::Log(MessageType::Error, err);
                  throw std::runtime_error(err);
            }

            auto texture = _world.try_get<Component::Gfx::Texture>(handle);
            if (!texture) {
                  const auto err = std::format("FrameGraph::BeginPass - this entity is not a texture entity.");
                  MessageManager::Log(MessageType::Error, err);
                  throw std::runtime_error(err);
            }

            auto extent = texture->GetExtent();
            if (_frameRenderingRenderArea.extent.width == 0 || _frameRenderingRenderArea.extent.height == 0) {
                  _frameRenderingRenderArea = {0, 0, extent.width, extent.height};
            } else {
                  if (extent.width != _frameRenderingRenderArea.extent.width || extent.height != _frameRenderingRenderArea.extent.height) {
                        const auto str = std::format("FrameGraph::BeginPass - Texture size mismatch, expected {}x{}, got {}x{}", _frameRenderingRenderArea.extent.width,
                        _frameRenderingRenderArea.extent.height, extent.width, extent.height);
                        MessageManager::Log(MessageType::Error, str);
                        throw std::runtime_error(str);
                  }
            }

            if (texture->IsTextureFormatColor()) {
                  // RenderTarget:

                  texture->BarrierLayout(_prev, GRAPHICS, ResourceUsage::RENDER_TARGET);

                  VkRenderingAttachmentInfo info {
                        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                        .imageView = texture->GetView(view_index),
                        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                        .loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
                        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                        .clearValue = {.color = {param.ClearColor.x, param.ClearColor.y, param.ClearColor.z, param.ClearColor.a}}
                  };

                  _frameRenderingColorAttachments.push_back(info);
                  render_info.colorAttachmentCount = _frameRenderingColorAttachments.size();
                  render_info.pColorAttachments = _frameRenderingColorAttachments.data();
            } else if (texture->IsTextureFormatDepthOnly()) {
                  // Depth:
                  texture->BarrierLayout(_prev, GRAPHICS, ResourceUsage::DEPTH_STENCIL);

                  _frameRenderingDepthAttachment = VkRenderingAttachmentInfo{
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
                  texture->BarrierLayout(_prev, GRAPHICS, ResourceUsage::DEPTH_STENCIL);

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
                  const auto err = std::format("FrameGraph::BeginPass - Invalid texture format, format = {}, create render pass failed.", GetVkFormatString(texture->GetFormat()));
                  MessageManager::Log(MessageType::Error, err);
                  throw std::runtime_error(err);
            }
      }

      render_info.renderArea = _frameRenderingRenderArea;
      vkCmdBeginRenderingKHR(_current, &render_info);
}

void LoFi::FrameGraph::EndRenderPass() {
      if (_passType != GRAPHICS) {
            const auto err = "FrameGraph::EndPass - Not in a pass, please use BeginPass in front of end, end pass failed.";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      _currentKernel = entt::null;
      vkCmdEndRendering(_current);
      EndSecondaryCommandBuffer();
      _passType = NONE;
}

void LoFi::FrameGraph::BindVertexBuffer(entt::entity buffer, uint32_t first_binding, uint32_t binding_count, size_t offset) {
      if (!_world.valid(buffer)) {
            const auto err = "FrameGraph::CmdBindVertexBuffer - Invalid buffer entity";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      auto buf = _world.try_get<Component::Gfx::Buffer>(buffer);
      if (!buf) {
            const auto err = "FrameGraph::CmdBindVertexBuffer - this enity is not a buffer";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      buf->BarrierLayout(_prev, GRAPHICS, ResourceUsage::VERTEX_BUFFER);
      vkCmdBindVertexBuffers(_current, first_binding, binding_count, buf->GetBufferPtr(), &offset);
}

void LoFi::FrameGraph::BindIndexBuffer(entt::entity index_buffer, size_t offset) const {
      if (!_world.valid(index_buffer)) {
            const auto err = "FrameGraph::BindIndexBuffer - Invalid buffer entity";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      auto ib = _world.try_get<Component::Gfx::Buffer>(index_buffer);
      if (!ib) {
            const auto err = "FrameGraph::BindIndexBuffer - this entity is not a buffer";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      ib->BarrierLayout(_prev, GRAPHICS, ResourceUsage::INDEX_BUFFER);
      vkCmdBindIndexBuffer(_current, ib->GetBuffer(), offset, VK_INDEX_TYPE_UINT32);
}

void LoFi::FrameGraph::DrawIndexedIndirect(entt::entity indirect_buffer, size_t offset, uint32_t draw_count, uint32_t stride) const {
      if (!_world.valid(indirect_buffer)) {
            const auto err = "FrameGraph::DrawIndexedIndirect - Invalid buffer entity";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      auto idr_buffer = _world.try_get<Component::Gfx::Buffer>(indirect_buffer);
      if (!idr_buffer) {
            const auto err = "FrameGraph::DrawIndexedIndirect - this entity is not a buffer";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      idr_buffer->BarrierLayout(_prev, GRAPHICS, ResourceUsage::INDIRECT_BUFFER);
      vkCmdDrawIndexedIndirect(_current, idr_buffer->GetBuffer(), offset, draw_count, stride);
}

void LoFi::FrameGraph::DrawIndex(uint32_t index_count, uint32_t instance_count, uint32_t first_index, int32_t vertex_offset, uint32_t first_instance) const {
      vkCmdDrawIndexed(_current, index_count, instance_count, first_index, vertex_offset, first_instance);
}

void LoFi::FrameGraph::Draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance) const {
      vkCmdDraw(_current, vertex_count, instance_count, first_vertex, first_instance);
}

void LoFi::FrameGraph::SetViewport(const VkViewport& viewport) const {
      vkCmdSetViewport(_current, 0, 1, &viewport);
}

void LoFi::FrameGraph::SetScissor(VkRect2D scissor) const {
      vkCmdSetScissor(_current, 0, 1, &scissor);
}

void LoFi::FrameGraph::SetViewportAuto(bool invert_y) const {
      if(invert_y) {
            const auto viewport = VkViewport{0, (float)_frameRenderingRenderArea.extent.height, (float)_frameRenderingRenderArea.extent.width, -(float)_frameRenderingRenderArea.extent.height, 0, 1};
            vkCmdSetViewport(_current, 0, 1, &viewport);
      } else {
            const auto viewport = VkViewport{0, 0, (float)_frameRenderingRenderArea.extent.width, (float)_frameRenderingRenderArea.extent.height, 0, 1};
            vkCmdSetViewport(_current, 0, 1, &viewport);
      }
}

void LoFi::FrameGraph::SetScissorAuto() const {
      const auto scissor = VkRect2D{0, 0, _frameRenderingRenderArea.extent.width, _frameRenderingRenderArea.extent.height};
      vkCmdSetScissor(_current, 0, 1, &scissor);
}

void LoFi::FrameGraph::PushConstant(entt::entity push_constant_buffer) const {
      if(!_world.valid(push_constant_buffer)) {
            const auto err = "FrameGraph::PushConstant - Invalid push constant buffer entity";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      const auto pcb = _world.try_get<Component::Gfx::PushConstantBuffer>(push_constant_buffer);
      if(!pcb) {
            const auto err = "FrameGraph::PushConstant - this entity is not a push constant buffer";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      pcb->CmdPushConstants(_current);
}

void LoFi::FrameGraph::AsSampledTexure(entt::entity texture, KernelType which_kernel_use) const {
      if(!_world.valid(texture)) {
            const auto err = "FrameGraph::AsSampledTex - Invalid texture entity";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      const auto tex = _world.try_get<Component::Gfx::Texture>(texture);
      if(!tex) {
            const auto err = "FrameGraph::AsSampledTex - this entity is not a texture";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      tex->BarrierLayout(_prev, which_kernel_use, ResourceUsage::SAMPLED);
}

void LoFi::FrameGraph::AsReadTexure(entt::entity texture, KernelType which_kernel_use) const {
      if(!_world.valid(texture)) {
            const auto err = "FrameGraph::AsReadTexure - Invalid texture entity";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      const auto tex = _world.try_get<Component::Gfx::Texture>(texture);
      if(!tex) {
            const auto err = "FrameGraph::AsReadTexure - this entity is not a texture";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      tex->BarrierLayout(_prev, which_kernel_use, ResourceUsage::READ_TEXTURE);
}

void LoFi::FrameGraph::AsWriteTexture(entt::entity texture, KernelType which_kernel_use) const {
      if(!_world.valid(texture)) {
            const auto err = "FrameGraph::AsWriteTexture - Invalid texture entity";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      const auto tex = _world.try_get<Component::Gfx::Texture>(texture);
      if(!tex) {
            const auto err = "FrameGraph::AsWriteTexture - this entity is not a texture";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      tex->BarrierLayout(_prev, which_kernel_use, ResourceUsage::WRITE_TEXTURE);
}

void LoFi::FrameGraph::AsWriteBuffer(entt::entity buffer, KernelType which_kernel_use) const {
      if(!_world.valid(buffer)) {
            const auto err = "FrameGraph::AsWriteBuffer - Invalid buffer entity";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      const auto tex = _world.try_get<Component::Gfx::Buffer>(buffer);
      if(!tex) {
            const auto err = "FrameGraph::AsWriteBuffer - this entity is not a buffer";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      tex->BarrierLayout(_prev, which_kernel_use, ResourceUsage::WRITE_BUFFER);
}

void LoFi::FrameGraph::AsReadBuffer(entt::entity buffer, KernelType which_kernel_use) const {
      if(!_world.valid(buffer)) {
            const auto err = "FrameGraph::AsReadBuffer - Invalid buffer entity";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      const auto tex = _world.try_get<Component::Gfx::Buffer>(buffer);
      if(!tex) {
            const auto err = "FrameGraph::AsReadBuffer - this entity is not a buffer";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }
      tex->BarrierLayout(_prev, which_kernel_use, ResourceUsage::READ_BUFFER);
}


void LoFi::FrameGraph::BeginSecondaryCommandBuffer() {
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

void LoFi::FrameGraph::EndSecondaryCommandBuffer() {
      vkEndCommandBuffer(_current);
      vkEndCommandBuffer(_prev);

      vkCmdExecuteCommands(_cmdBuffer, 1, &_prev);
      vkCmdExecuteCommands(_cmdBuffer, 1, &_current);
      _current = _cmdBuffer;
      _prev = _cmdBuffer;
}

void LoFi::FrameGraph::BindKernel(entt::entity kernel) {
      if (_currentKernel == kernel) return;

      if (!_world.valid(kernel)) {
            const auto err = "FrameGraph::BindKernel - Invalid graphics kernel entity";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      const auto kernel_ptr = _world.try_get<Component::Gfx::Kernel>(kernel);
      if (!kernel_ptr) {
            const auto err = "FrameGraph::BindKernel - this entity is not a kernel or kernel instance entity.";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      if (kernel_ptr->IsComputeKernel()) {
            if (_passType != COMPUTE) {
                  const auto err = "FrameGraph::BindKernel - Compute kernel must be used in Compute Pass.";
                  MessageManager::Log(MessageType::Error, err);
                  throw std::runtime_error(err);
            }
            vkCmdBindPipeline(_current, VK_PIPELINE_BIND_POINT_COMPUTE, kernel_ptr->GetPipeline());
            vkCmdBindDescriptorSets(_current, VK_PIPELINE_BIND_POINT_COMPUTE, kernel_ptr->GetPipelineLayout(),
            0, 1, &GfxContext::Get()->_bindlessDescriptorSet, 0, nullptr);
            kernel_ptr->CmdPushConstants(_current);
      } else if (kernel_ptr->IsGraphicsKernel()) {
            if (_passType != GRAPHICS) {
                  const auto err = "FrameGraph::BindKernel - Graphics kernel must be used in Render Pass.";
                  MessageManager::Log(MessageType::Error, err);
                  throw std::runtime_error(err);
            }
            vkCmdBindPipeline(_current, VK_PIPELINE_BIND_POINT_GRAPHICS, kernel_ptr->GetPipeline());
            vkCmdBindDescriptorSets(_current, VK_PIPELINE_BIND_POINT_GRAPHICS, kernel_ptr->GetPipelineLayout(), 0, 1, &GfxContext::Get()->_bindlessDescriptorSet, 0, nullptr);
            const auto viewport = VkViewport{0, (float)_frameRenderingRenderArea.extent.height, (float)_frameRenderingRenderArea.extent.width, -(float)_frameRenderingRenderArea.extent.height, 0, 1};
            vkCmdSetViewport(_current, 0, 1, &viewport);
            const auto scissor = VkRect2D{0, 0, _frameRenderingRenderArea.extent.width, _frameRenderingRenderArea.extent.height};
            vkCmdSetScissor(_current, 0, 1, &scissor);
            kernel_ptr->CmdPushConstants(_current);
      } else {
            throw std::runtime_error("FrameGraph::BindKernel - this kernel is not a graphics kernel or compute kernel.");
      }

      _currentKernel = kernel;
}

void LoFi::FrameGraph::ExpandSecondaryCommandBuffer() {
      VkCommandBufferAllocateInfo second_command_buffer_ai{};
      second_command_buffer_ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
      second_command_buffer_ai.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
      second_command_buffer_ai.commandPool = _secondaryCmdPool;
      second_command_buffer_ai.commandBufferCount = 16;

      std::vector<VkCommandBuffer> new_buffers(16); //expand 16

      if (vkAllocateCommandBuffers(volkGetLoadedDevice(), &second_command_buffer_ai, new_buffers.data()) != VK_SUCCESS) {
            const auto err = "FrameGraph::ExpandSecondaryCommandBuffer - Failed to allocate secondary command buffers";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      _secondaryCmdBufsFree.reserve(_secondaryCmdBufsFree.size() + 16);
      for (auto& buffer : new_buffers) {
            _secondaryCmdBufsFree.push_back(buffer);
      }
}

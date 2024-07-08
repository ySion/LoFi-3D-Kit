//
// Created by Arzuo on 2024/7/8.
//
#include "FrameGraph.h"
#include "Message.h"
#include "Context.h"
#include "Components/Texture.h"
#include "Components/Kernel.h"
#include "Components/KernelInstance.h"

using namespace LoFi::Internal;

LoFi::FrameGraph::~FrameGraph() = default;

LoFi::FrameGraph::FrameGraph(VkCommandBuffer cmd_buf, VkCommandPool cmd_pool) : _cmdBuffer(cmd_buf), _cmdPool(cmd_pool), _world(*volkGetLoadedEcsWorld()) {
      VkCommandBufferAllocateInfo second_command_buffer_ai{};
      second_command_buffer_ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
      second_command_buffer_ai.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
      second_command_buffer_ai.commandPool = _cmdPool;
      second_command_buffer_ai.commandBufferCount = 64;

      _secondaryCmdBufsFree.resize(64);

      if (vkAllocateCommandBuffers(volkGetLoadedDevice(), &second_command_buffer_ai, _secondaryCmdBufsFree.data()) != VK_SUCCESS) {
            const auto err = "Context::ExpandSecondaryCommandBuffer - Failed to allocate secondary command buffers";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }
}

void LoFi::FrameGraph::BeginFrame() {

      for (auto used_cmd_buf : _secondaryCmdBufsUsed) {

            if (vkResetCommandBuffer(used_cmd_buf, 0) != VK_SUCCESS) {
                  const auto err = "FrameGraph::Reset Failed to reset secondary command buffer";
                  MessageManager::Log(MessageType::Error, err);
                  throw std::runtime_error(err);
            }

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

void LoFi::FrameGraph::BeginPass(const std::vector<RenderPassBeginArgument>& textures) {

      BeginSecondaryCommandBuffer();

      if (textures.empty()) {
            const auto err = "FrameGraph::BeginRenderPass - Empty textures, create render pass failed.";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      _frameRenderingRenderArea = {};

      VkRenderingInfoKHR render_info = {
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
      for (const auto& entity : textures) {
            entt::entity handle = entity.TextureHandle;
            uint32_t view_index = entity.ViewIndex;
            bool clear = entity.ClearBeforeRendering;
            if (!_world.valid(handle)) {
                  const auto err = std::format("FrameGraph::BeginPass - Invalid texture entity.");
                  MessageManager::Log(MessageType::Error, err);
                  throw std::runtime_error(err);
            }

            auto texture = _world.try_get<Component::Texture>(handle);
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

                  const VkRenderingAttachmentInfo info{
                        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                        .imageView = texture->GetView(view_index),
                        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                        .loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
                        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                        .clearValue = {.color = {0.0f, 0.0f, 0.0f, 1.0f}}
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
            } else if (texture->IsTextureFormatDepthStencil()) {
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

void LoFi::FrameGraph::EndPass() {
      vkCmdEndRendering(_current);
      EndSecondaryCommandBuffer();
}

void LoFi::FrameGraph::BindVertexBuffer(entt::entity buffer, size_t offset) const {
      if (!_world.valid(buffer)) {
            const auto err = "Context::CmdBindVertexBuffer - Invalid buffer entity";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      auto buf = _world.try_get<Component::Buffer>(buffer);
      if (!buf) {
            const auto err = "Context::CmdBindVertexBuffer - this enity is not a buffer";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      buf->BarrierLayout(_prev, GRAPHICS, ResourceUsage::VERTEX_BUFFER);
      vkCmdBindVertexBuffers(_current, 0, 1, buf->GetBufferPtr(), &offset);
}

void LoFi::FrameGraph::DrawIndex(entt::entity index_buffer, size_t offset, std::optional<uint32_t> index_count) const {
      if (!_world.valid(index_buffer)) {
            const auto err = "Context::CmdDrawIndex - Invalid buffer entity";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      auto ib = _world.try_get<Component::Buffer>(index_buffer);
      if (!ib) {
            const auto err = "Context::CmdDrawIndex - this entity is not a buffer";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      ib->BarrierLayout(_prev, GRAPHICS, ResourceUsage::INDEX_BUFFER);

      vkCmdBindIndexBuffer(_current, ib->GetBuffer(), offset, VK_INDEX_TYPE_UINT32);
      uint32_t idx_count = std::min(index_count.value_or(std::numeric_limits<uint32_t>::max()), (uint32_t)(ib->GetSize() / sizeof(uint32_t)));
      vkCmdDrawIndexed(_current, idx_count, 1, 0, 0, 0);
}

void LoFi::FrameGraph::BeginSecondaryCommandBuffer() {
      if(_secondaryCmdBufsFree.size() < 2) {
            ExpandSecondaryCommandBuffer();
      }

      _current = _secondaryCmdBufsFree.back();
      _secondaryCmdBufsFree.pop_back();
      _secondaryCmdBufsUsed.push_back(_current);

      _prev = _secondaryCmdBufsFree.back();
      _secondaryCmdBufsFree.pop_back();
      _secondaryCmdBufsUsed.push_back(_prev);

      constexpr VkCommandBufferInheritanceInfo inheritance_info {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
            .pNext = nullptr,
            .renderPass = nullptr,
            .subpass = 0,
            .framebuffer = nullptr,
            .occlusionQueryEnable = false,
            .queryFlags = 0,
            .pipelineStatistics = 0
      };

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
      _current = nullptr;
      _prev = nullptr;
}

void LoFi::FrameGraph::BindKernel(entt::entity kernel) {
      if (!_world.valid(kernel)) {
            const auto err = "FrameGraph::BindKernel - Invalid graphics kernel entity";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      const auto kernel_instance_ptr = _world.try_get<Component::KernelInstance>(kernel);
      if(kernel_instance_ptr) {
            kernel = kernel_instance_ptr->GetKernel();
      }

      const auto kernel_ptr = _world.try_get<Component::Kernel>(kernel);
      if(!kernel_ptr) {
            throw std::runtime_error("FrameGraph::BindKernel - this entity is not a kernel or kernel instance entity.");
      }

      bool is_compute = kernel_ptr->IsComputeKernel();

      if(is_compute) {


            vkCmdBindPipeline(_current, VK_PIPELINE_BIND_POINT_COMPUTE, kernel_ptr->GetPipeline());
            vkCmdBindDescriptorSets(_current, VK_PIPELINE_BIND_POINT_COMPUTE, kernel_ptr->GetPipelineLayout(),
                  0, 1, &Context::Get()->_bindlessDescriptorSet, 0, nullptr);

      } else if(kernel_ptr->IsGraphicsKernel()) {
            vkCmdBindPipeline(_current, VK_PIPELINE_BIND_POINT_GRAPHICS, kernel_ptr->GetPipeline());
            vkCmdBindDescriptorSets(_current, VK_PIPELINE_BIND_POINT_GRAPHICS, kernel_ptr->GetPipelineLayout(), 0, 1, &Context::Get()->_bindlessDescriptorSet, 0, nullptr);
            const auto viewport = VkViewport{0, (float)_frameRenderingRenderArea.extent.height, (float)_frameRenderingRenderArea.extent.width, -(float)_frameRenderingRenderArea.extent.height, 0, 1};
            vkCmdSetViewport(_current, 0, 1, &viewport);
            const auto scissor = VkRect2D{0, 0, _frameRenderingRenderArea.extent.width, _frameRenderingRenderArea.extent.height};
            vkCmdSetScissor(_current, 0, 1, &scissor);
      } else {
            throw std::runtime_error("FrameGraph::BindKernel - this kernel is not a graphics kernel or compute kernel.");
      }

      if(kernel_instance_ptr) {
            kernel_instance_ptr->GenerateResourcesBarrier(_prev);
            kernel_instance_ptr->PushParameterTable(_current);
            if(is_compute) {
                  vkCmdExecuteCommands(_cmdBuffer, 1, &_prev);
                  vkCmdExecuteCommands(_cmdBuffer, 1, &_current);
                  _current = nullptr;
                  _prev = nullptr;
            }
      }
}

void LoFi::FrameGraph::ExpandSecondaryCommandBuffer() {

      VkCommandBufferAllocateInfo second_command_buffer_ai{};
      second_command_buffer_ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
      second_command_buffer_ai.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
      second_command_buffer_ai.commandPool = _cmdPool;
      second_command_buffer_ai.commandBufferCount = 16;

      std::vector<VkCommandBuffer> new_buffers(16); //expand 16

      if (vkAllocateCommandBuffers(volkGetLoadedDevice(), &second_command_buffer_ai, new_buffers.data()) != VK_SUCCESS) {
            const auto err = "Context::ExpandSecondaryCommandBuffer - Failed to allocate secondary command buffers";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      _secondaryCmdBufsFree.reserve(_secondaryCmdBufsFree.size() + 16);
      for (auto& buffer : new_buffers) {
            _secondaryCmdBufsFree.push_back(buffer);
      }
}

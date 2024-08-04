//
// Created by Arzuo on 2024/8/3.
//

#pragma once
#include "Helper.h"
#include "GfxComponents/Buffer.h"

namespace LoFi {

      namespace Component::Gfx {
            class Buffer;
            class Texture;
      }

      class RenderNodeFrameCommand {
      public:
            NO_COPY_MOVE_CONS(RenderNodeFrameCommand);

            explicit RenderNodeFrameCommand(const std::string& name);

            ~RenderNodeFrameCommand();

            void BeginSecondaryCommandBuffer();

            void EndSecondaryCommandBuffer() const;

            void Reset();

            [[nodiscard]] VkCommandBuffer GetCommandBufferPrev() const { return _prev; };

            [[nodiscard]] VkCommandBuffer GetCommandBufferCurr() const { return _current; };

            std::vector<VkCommandBuffer>& GetSecondaryCommandBuffers() { return _secondaryCmdBufsUsed; }

      private:
            void ExpandSecondaryCommandBuffer();

            VkCommandPool _cmdPool {};

            std::vector<VkCommandBuffer> _secondaryCmdBufsFree{};

            std::vector<VkCommandBuffer> _secondaryCmdBufsUsed{};

            VkCommandBuffer _prev {};

            VkCommandBuffer _current {};

            std::string _nodeName{};
      };

      struct RenderNodeBarrierVectorTexture {
            Component::Gfx::Texture* texture {};
            GfxEnumKernelType first_barrier_kernel_type {};
            GfxEnumResourceUsage first_barrier_usage {};
      };

      struct RenderNodeBarrierVectorBuffer {
            Component::Gfx::Buffer* buffer {};
            GfxEnumKernelType first_barrier_kernel_type {};
            GfxEnumResourceUsage first_barrier_usage {};
      };

      struct RenderNodeBarrierMapItemTexture {
            GfxEnumKernelType kernel_type;
            GfxEnumResourceUsage usage;
      };

      struct RenderNodeBarrierMapItemBuffer {
            GfxEnumKernelType kernel_type;
            GfxEnumResourceUsage usage;
      };

      class RenderNode {
      public:

            NO_COPY_MOVE_CONS(RenderNode);

            explicit RenderNode(entt::entity id, const std::string& name);

            ~RenderNode();

            [[nodiscard]] const std::string& GetNodeName() const { return _nodeName; }

            [[nodiscard]] ResourceHandle GetHandle() const { return {GfxEnumResourceType::RenderGraphNode, _id}; }

            [[nodiscard]] bool IsEmptyNode() const { return GetCurrentFrameCommand()->GetSecondaryCommandBuffers().empty(); }

            //Node After

            void WaitNodes(const GfxInfoRenderNodeWait& param);

            void CmdBeginComputePass();

            void CmdEndComputePass();

            void CmdComputeDispatch(uint32_t x, uint32_t y, uint32_t z) const;

            void CmdBeginRenderPass(const GfxParamBeginRenderPass& param);

            void CmdEndRenderPass();

            void CmdBindKernel(ResourceHandle kernel);

            void CmdBindVertexBuffer(ResourceHandle vertex_bufer, uint32_t first_binding = 0, uint32_t binding_count = 1, size_t offset = 0);

            void CmdBindIndexBuffer(ResourceHandle index_buffer, size_t offset = 0);

            void CmdDrawIndexedIndirect(ResourceHandle indirect_buffer, size_t offset, uint32_t draw_count, uint32_t stride);

            void CmdDrawIndex(uint32_t index_count, uint32_t instance_count = 1, uint32_t first_index = 0, int32_t vertex_offset = 0, uint32_t first_instance = 0) const;

            void CmdDraw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex = 0, uint32_t first_instance = 0) const;

            void CmdSetViewport(const VkViewport& viewport) const;

            void CmdSetScissor(VkRect2D scissor) const;

            void CmdSetViewportAuto(bool invert_y = true) const;

            void CmdSetScissorAuto() const;

            void CmdPushConstant(GfxInfoKernelLayout kernel_layout, const void* data, size_t data_size) const;

            void CmdAsSampledTexure(ResourceHandle texture, GfxEnumKernelType which_kernel_use = GfxEnumKernelType::OUT_OF_KERNEL);

            void CmdAsReadTexure(ResourceHandle texture, GfxEnumKernelType which_kernel_use = GfxEnumKernelType::OUT_OF_KERNEL);

            void CmdAsWriteTexture(ResourceHandle texture, GfxEnumKernelType which_kernel_use = GfxEnumKernelType::OUT_OF_KERNEL);

            void CmdAsWriteBuffer(ResourceHandle buffer, GfxEnumKernelType which_kernel_use = GfxEnumKernelType::OUT_OF_KERNEL);

            void CmdAsReadBuffer(ResourceHandle buffer, GfxEnumKernelType which_kernel_use = GfxEnumKernelType::OUT_OF_KERNEL);

      private:
            void EmitCommands(VkCommandBuffer primary_cmdbuf) const;

            void SetFrameIndex(uint32_t frame_index) { _frameIndex = frame_index % 3;}

            void PrepareFrame();

            friend class FrameGraph;
      private:

            static void CmdBarrierTexture(VkCommandBuffer buf, Component::Gfx::Texture* texture, GfxEnumKernelType old_kernel_type, GfxEnumResourceUsage old_usage,
                  GfxEnumKernelType new_kernel_type, GfxEnumResourceUsage new_usage, std::string_view node_name);

            static void CmdBarrierBuffer(VkCommandBuffer buf, Component::Gfx::Buffer* buffer, GfxEnumKernelType old_kernel_type, GfxEnumResourceUsage old_usage,
                  GfxEnumKernelType new_kernel_type, GfxEnumResourceUsage new_usage, std::string_view node_name);

            void BarrierTexture(Component::Gfx::Texture* texture, GfxEnumKernelType new_kernel_type, GfxEnumResourceUsage new_usage);

            void BarrierBuffer(Component::Gfx::Buffer* buffer, GfxEnumKernelType new_kernel_type, GfxEnumResourceUsage new_usage);

            void BeginSecondaryCommandBuffer();

            void EndSecondaryCommandBuffer();

      private:
            [[nodiscard]] RenderNodeFrameCommand* GetCurrentFrameCommand() const { return _frameCommand[_frameIndex].get(); }

            entt::entity _id;

            std::string _nodeName{};

      private:
            ResourceHandle _currentKernel {};

            GfxEnumKernelType _currentPassType = GfxEnumKernelType::OUT_OF_KERNEL;

            VkCommandBuffer _prev {};

            VkCommandBuffer _current {};

      private:
            std::vector<RenderNodeBarrierVectorTexture> _beginBarrierTexture{};

            std::vector<RenderNodeBarrierVectorBuffer> _beginBarrierBuffer{};

            entt::dense_map<Component::Gfx::Texture*, RenderNodeBarrierMapItemTexture> _barrierTableTexture{};

            entt::dense_map<Component::Gfx::Buffer*, RenderNodeBarrierMapItemBuffer> _barrierTableBuffer{};

      private:
            uint32_t _frameIndex = 0;

            VkRect2D _frameRenderingRenderArea{};

            std::unique_ptr<RenderNodeFrameCommand> _frameCommand[3]{};

      private:
            std::vector<std::string> _waitNodes {};

            std::vector<RenderNode*> _nodeAfter{};
      };

}


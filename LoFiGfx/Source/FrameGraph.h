//
// Created by Arzuo on 2024/7/8.
//

#pragma once
#include "Helper.h"

namespace LoFi {

    class FrameGraph {
    public:

        NO_COPY_MOVE_CONS(FrameGraph);

        ~FrameGraph();

        explicit FrameGraph(VkCommandBuffer cmd_buf);

    public:

        void BeginFrame();

        void EndFrame() const;

        void BeginComputePass();

        void EndComputePass();

        void ComputeDispatch(uint32_t x, uint32_t y, uint32_t z) const;

        void BeginRenderPass(const std::vector<RenderPassBeginArgument>& textures);

        void EndRenderPass();

        void BindKernel(entt::entity kernel);

        void BindVertexBuffer(entt::entity buffer, uint32_t first_binding, uint32_t binding_count, size_t offset);

        void BindIndexBuffer(entt::entity index_buffer, size_t offset) const;

        void DrawIndexedIndirect(entt::entity indirect_bufer, size_t offset, uint32_t draw_count, uint32_t stride) const;

        void DrawIndex(uint32_t index_count, uint32_t instance_count = 1, uint32_t first_index = 0, int32_t vertex_offset = 0, uint32_t first_instance = 0) const;

        void Draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance) const;

        void SetViewport(const VkViewport& viewport) const;

        void SetScissor(VkRect2D scissor) const;

        void SetViewportAuto(bool invert_y = true) const;

        void SetScissorAuto() const;

        void PushConstant(entt::entity push_constant_buffer) const;

        void AsSampledTexure(entt::entity texture, KernelType which_kernel_use = GRAPHICS) const;

        void AsReadTexure(entt::entity texture, KernelType which_kernel_use = COMPUTE) const;

        void AsWriteTexture(entt::entity texture, KernelType which_kernel_use = COMPUTE) const;

        void AsWriteBuffer(entt::entity buffer, KernelType which_kernel_use = COMPUTE) const;

        void AsReadBuffer(entt::entity buffer, KernelType which_kernel_use = COMPUTE) const;

    public:
        [[nodiscard]] VkCommandBuffer GetCommandBuffer() const { return _cmdBuffer; }

    private:

        void BeginSecondaryCommandBuffer();

        void EndSecondaryCommandBuffer();

        void ExpandSecondaryCommandBuffer();

        VkCommandBuffer _cmdBuffer;

        VkCommandPool _secondaryCmdPool {};

        std::vector<VkCommandBuffer> _secondaryCmdBufsFree{};

        std::vector<VkCommandBuffer> _secondaryCmdBufsUsed{};

        VkCommandBuffer _prev {};

        VkCommandBuffer _current {};

        entt::registry& _world;

        VkRect2D _frameRenderingRenderArea{};
    private:
        entt::entity _currentKernel = entt::null;

        KernelType _passType = NONE;
    };

}

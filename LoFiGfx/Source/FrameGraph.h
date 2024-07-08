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

        FrameGraph(VkCommandBuffer cmd_buf, VkCommandPool cmd_pool);

    public:

        void BeginFrame();

        void EndFrame() const;

        void BeginPass(const std::vector<RenderPassBeginArgument>& textures);

        void EndPass();

        void BindKernel(entt::entity kernel);

        void BindVertexBuffer(entt::entity buffer, size_t offset) const;

        void DrawIndex(entt::entity index_buffer, size_t offset = 0, std::optional<uint32_t> index_count = {}) const;

    public:
        [[nodiscard]] VkCommandBuffer GetCommandBuffer() const { return _cmdBuffer; }

    private:

        void BeginSecondaryCommandBuffer();

        void EndSecondaryCommandBuffer();

        void ExpandSecondaryCommandBuffer();

        VkCommandBuffer _cmdBuffer;

        VkCommandPool _cmdPool;

        std::vector<VkCommandBuffer> _secondaryCmdBufsFree{};

        std::vector<VkCommandBuffer> _secondaryCmdBufsUsed{};

        VkCommandBuffer _prev {};

        VkCommandBuffer _current {};

        entt::registry& _world;

        VkRect2D _frameRenderingRenderArea{};
    };

}
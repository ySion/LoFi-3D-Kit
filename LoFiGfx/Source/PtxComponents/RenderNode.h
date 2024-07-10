//
// Created by Arzuo on 2024/7/10.
//

#pragma once

#include "../FrameGraph.h"
#include "../Helper.h"

namespace LoFi::Component::Ptx {

      struct PtrRenderNodeCmdDrawRange {
            uint32_t StartIndex;
            uint32_t EndIndex;
      };

      enum class PtxRenderNodeCommand {
            DRAW,
            SCISSOR,
            SCISSOR_AUTO,
            NEW_LAYER,
      };

      struct PtxRenderNodeCommandInfo {
            PtxRenderNodeCommand Command;
            union {
                  PtrRenderNodeCmdDrawRange DrawRange;
                  VkRect2D ScissorInfo;
            };
      };

      struct PtxRenderNodeVertexPack {
            float pos_x;
            float pos_y;
            float uv_x;
            float uv_y;
            float color_r;
            float color_g;
            float color_b;
            float color_a;
      };

      class RenderNode {
      public:
            NO_COPY_MOVE_CONS(RenderNode);

            explicit RenderNode(entt::entity id);

            ~RenderNode();

            void Begin();

            void Generate();

            void EmitCommand(const std::optional<VkViewport>& viewport = std::nullopt) const;

            void SetVirtualCanvasSize(uint32_t w, uint32_t h);

            void Reset();

            void PushScissor(int x, int y, uint32_t width, uint32_t height);

            void PopScissor();

            void DrawRect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, float r, float g, float b, float a);

            void NewLayer();

      private:

            void CalcVirtualCanvasSize();

            void CalcVertexList();

            entt::entity _id;

            entt::entity _vertexBuffer = entt::null;

            entt::entity _indexBuffer = entt::null;

            uint32_t _virtualCanvasWidth {1920};

            uint32_t _virtualCanvasHeight {1080};

            std::vector<PtxRenderNodeVertexPack> _vertexList{};

            std::vector<uint32_t> _indexList{};

            std::vector<PtxRenderNodeCommandInfo> _commands{};

            std::vector<VkRect2D> _scissorStack{};
      };
}

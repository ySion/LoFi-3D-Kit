//
// Created by Arzuo on 2024/7/10.
//

#pragma once

#include "../Third/earcut/earcut.hpp"
#include "../Third/clipper2/clipper.h"

#include "../FrameGraph.h"
#include "../Helper.h"

namespace mapbox {
      namespace util {
            template <>
            struct nth<0, Clipper2Lib::Point64> {
                  inline static double get(const Clipper2Lib::Point64& t) {
                        return (double)t.x;
                  };
            };

            template <>
            struct nth<1, Clipper2Lib::Point64> {
                  inline static double get(const Clipper2Lib::Point64& t) {
                        return (double)t.y;
                  };
            };
      } // namespace util
}

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

      struct __attribute__((packed)) PtxRenderNodeVertexPack {
            glm::i16vec2 pos;
            glm::i16vec2 uv;
            glm::i8vec4 color;
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

            void DrawRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t r, uint8_t g, uint8_t b, uint8_t a);

            void DrawPath(std::span<glm::ivec2> pos, bool closed, uint32_t thickness, uint32_t r, uint32_t g, uint32_t b, uint32_t a, uint32_t expend = 1);

            void NewLayer();

            static void GetPathSegmentLineTangent(std::span<glm::ivec2> pos, bool closed, std::vector<glm::vec2>& tang);

            static void GetPathSegmentLineNormal(std::span<glm::ivec2> pos, bool closed,
                  std::vector<glm::vec2>& normal, std::vector<glm::vec2>& tang, std::vector<bool>& sharp_angle);

            static void GetPathSegmentJoltPointNormal(std::span<glm::ivec2> pos, bool closed,
                  std::vector<glm::vec2>& line_normal, std::vector<glm::vec2>& line_tang,
                  std::vector<bool>& angle_sharp, std::vector<glm::vec2>& jolts_normal, std::vector<float>& half_angle);

      private:
            void ExtrudeUp(std::span<glm::ivec2> pos, bool closed, uint32_t distance, uint32_t expend_layers, std::vector<glm::vec2>& vertex, std::vector<uint32_t>& index);

            void CalcVirtualCanvasSize();

            void CalcVertexList();

            inline PtxRenderNodeVertexPack MapPos(uint16_t x, uint16_t y, uint16_t u, uint16_t v, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
                  return {
                        .pos = glm::i16vec2((glm::vec2(x, y) / _virtualCanvasSize) * 65535.0f),
                        .uv = glm::i16vec2(glm::vec2(u, v) * 65535.0f),
                        .color = glm::i8vec4(r, g, b, a)
                  };
            }

            inline PtxRenderNodeVertexPack MapPos(glm::vec2 pos, glm::vec2 uv, glm::i8vec4 color) {
                  return {
                        .pos = glm::i16vec2((pos / _virtualCanvasSize) * 65535.0f),
                        .uv = glm::i16vec2(uv * 65535.0f),
                        .color = color
                  };
            }

            entt::entity _id;

            entt::entity _vertexBuffer = entt::null;

            entt::entity _indexBuffer = entt::null;

            std::vector<PtxRenderNodeVertexPack> _vertexList{};

            std::vector<uint32_t> _indexList{};

            std::vector<PtxRenderNodeCommandInfo> _commands{};

            std::vector<VkRect2D> _scissorStack{};

            glm::vec2 _virtualCanvasSize;

      private:
            //cache
            std::vector<glm::vec2> line_tang;
            std::vector<glm::vec2> line_norm;
            std::vector<glm::vec2> jolt_norm;
            std::vector<bool> angle_sharp;
            std::vector<float> half_angle;

            std::vector<glm::vec2> expended_vertex;
            std::vector<uint32_t> indexs;
      };
}

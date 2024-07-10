//
// Created by Arzuo on 2024/7/10.
//

#include "RenderNode.h"

#include "../GfxContext.h"
#include "../PfxContext.h"
#include "../Message.h"

using namespace LoFi::Component::Ptx;

RenderNode::RenderNode(entt::entity id) : _id(id) {
      if(!PfxContext::Get()->_world.valid(id)) {
            const auto error = "Invalid entity id";
            MessageManager::Log(MessageType::Error, error);
            throw std::runtime_error(error);
      }
}

RenderNode::~RenderNode() {
      const auto& gfx = GfxContext::Get();
      if (_vertexBuffer != entt::null) {
            gfx->DestroyHandle(_vertexBuffer);
      }

      if(_indexBuffer != entt::null) {
            gfx->DestroyHandle(_indexBuffer);
      }
}

void RenderNode::Generate() {
      const auto& gfx = GfxContext::Get();
      if (_vertexBuffer == entt::null) {
            _vertexBuffer = gfx->CreateBuffer(_vertexList);
      } else {
            gfx->SetBufferData(_vertexBuffer, _vertexList.data(), _vertexList.size() * sizeof(PtxRenderNodeVertexPack));
      }

      if(_indexBuffer == entt::null) {
            _indexBuffer = gfx->CreateBuffer(_indexList);
      } else {
            gfx->SetBufferData(_indexBuffer, _indexList.data(), _indexList.size() * sizeof(uint32_t));
      }
}

void RenderNode::SetVirtualCanvasSize(uint32_t width, uint32_t height) {
      _virtualCanvasWidth = width;
      _virtualCanvasHeight = height;
}

void RenderNode::Reset() {
      _vertexList.clear();
      _commands.clear();
      _scissorStack.clear();
}

void RenderNode::EmitCommand(const std::optional<VkViewport>& viewport) const {
      const auto& gfx = GfxContext::Get();
      const auto& pfx = PfxContext::Get();
      if(!_commands.empty() && viewport.has_value()) {
            gfx->CmdSetViewport(viewport.value());
      } else {
            gfx->CmdSetViewportAuto();
      }
      entt::entity current_kernel_instance = entt::null;
      for (auto& cmd : _commands) {
            switch (cmd.Command) {
                  case PtxRenderNodeCommand::DRAW:
                        if(current_kernel_instance != pfx->_kernelNormalInstance) {
                              current_kernel_instance = pfx->_kernelNormalInstance;
                        }
                        gfx->CmdBindKernel(current_kernel_instance);
                        gfx->CmdBindVertexBuffer(_vertexBuffer);
                        gfx->CmdDrawIndex(_indexBuffer, cmd.DrawRange.StartIndex, cmd.DrawRange.EndIndex - cmd.DrawRange.StartIndex);
                        break;
                  case PtxRenderNodeCommand::SCISSOR:
                        gfx->CmdSetScissor(cmd.ScissorInfo.offset.x, cmd.ScissorInfo.offset.y, cmd.ScissorInfo.extent.width, cmd.ScissorInfo.extent.height);
                        break;
                  case PtxRenderNodeCommand::SCISSOR_AUTO:
                        gfx->CmdSetScissorAuto();
                        break;
            }
      }

      //End
      if(!_commands.empty() && viewport.has_value()) {
            gfx->CmdSetViewportAuto();
      }
}

void RenderNode::PushScissor(int x, int y, uint32_t width, uint32_t height) {
      auto temp = VkRect2D{x, y, width, height};

      _commands.emplace_back(PtxRenderNodeCommand::SCISSOR);
      _commands.back().ScissorInfo = temp;

      _scissorStack.emplace_back(temp);
}

void RenderNode::PopScissor() {
      if (_scissorStack.empty()) {
            const auto error = "Scissor stack is empty, cannot pop";
            MessageManager::Log(MessageType::Error, error);
            throw std::runtime_error(error);
      } else {
            _scissorStack.pop_back();
            if (_scissorStack.empty()) {
                  _commands.emplace_back(PtxRenderNodeCommand::SCISSOR_AUTO);
            } else {
                  auto prev_scissor = _scissorStack.back();
                  _commands.emplace_back(PtxRenderNodeCommand::SCISSOR);
                  _commands.back().ScissorInfo = prev_scissor;
            }
      }
}

void RenderNode::DrawRect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, float r, float g, float b, float a) {

      //vec2 pos, vec2 uv, vec4 color
      //vec4 pos_uv, vec4 color
      const std::array<PtxRenderNodeVertexPack, 4> square_vt = {
           PtxRenderNodeVertexPack {2 * ((float)x / (float)_virtualCanvasWidth) - 1,                  2 * ((float)(y + height) / (float)_virtualCanvasHeight) - 1,     (float)0, (float)0, r, g, b, a},     // top left
           PtxRenderNodeVertexPack {2 * ((float)(x + width) / (float)_virtualCanvasWidth) - 1,        2 * ((float)(y + height) / (float)_virtualCanvasHeight) - 1,     (float)0, (float)0, r, g, b, a},     // top right
           PtxRenderNodeVertexPack {2 * ((float)(x + width) / (float)_virtualCanvasWidth) - 1,        2 * ((float)y / (float)_virtualCanvasHeight) - 1,                (float)0, (float)0, r, g, b, a},     // botom left
           PtxRenderNodeVertexPack {2 * ((float)x / (float)_virtualCanvasWidth) - 1,                  2 * ((float)y / (float)_virtualCanvasHeight) - 1,                (float)0, (float)0, r, g, b, a},      // bottom right
      };

      const auto current_vertex_count = (uint32_t)_vertexList.size();
      const auto current_index_count = (uint32_t)_indexList.size();

      const std::array square_id = {
            0 + current_vertex_count , 1 + current_vertex_count, 2 + current_vertex_count,
            0 + current_vertex_count , 2 + current_vertex_count, 3 + current_vertex_count
      };

      _vertexList.resize(_vertexList.size() + square_vt.size());
      _indexList.resize(_indexList.size() + square_id.size());

      memcpy(&_vertexList.at(current_vertex_count), square_vt.data(), square_vt.size() * sizeof(PtxRenderNodeVertexPack));
      memcpy(&_indexList.at(current_index_count), square_id.data(), square_id.size() * sizeof(uint32_t));

      if (_commands.empty() || _commands.back().Command != PtxRenderNodeCommand::DRAW) {
            _commands.emplace_back(PtxRenderNodeCommand::DRAW);
            _commands.back().DrawRange.StartIndex = current_index_count;
      }

      _commands.back().DrawRange.EndIndex = _indexList.size();
}

void RenderNode::NewLayer() {
      if (!_commands.empty() && _commands.back().Command != PtxRenderNodeCommand::DRAW) {
            _commands.emplace_back(PtxRenderNodeCommand::NEW_LAYER);
      }
}

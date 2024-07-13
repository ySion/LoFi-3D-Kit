//
// Created by Arzuo on 2024/7/10.
//

#include "RenderNode.h"

#include "../GfxContext.h"
#include "../PfxContext.h"
#include "../Message.h"

using namespace LoFi::Component::Ptx;

RenderNode::RenderNode(entt::entity id) : _id(id) {
      if (!PfxContext::Get()->_world.valid(id)) {
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

      if (_indexBuffer != entt::null) {
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

      if (_indexBuffer == entt::null) {
            _indexBuffer = gfx->CreateBuffer(_indexList);
      } else {
            gfx->SetBufferData(_indexBuffer, _indexList.data(), _indexList.size() * sizeof(uint32_t));
      }
}

void RenderNode::SetVirtualCanvasSize(uint32_t width, uint32_t height) {
      _virtualCanvasSize = {width , height};
}

void RenderNode::Reset() {
      _vertexList.clear();
      _indexList.clear();
      _commands.clear();
      _scissorStack.clear();
}

void RenderNode::EmitCommand(const std::optional<VkViewport>& viewport) const {
      const auto& gfx = GfxContext::Get();
      const auto& pfx = PfxContext::Get();
      if (!_commands.empty() && viewport.has_value()) {
            gfx->CmdSetViewport(viewport.value());
      } else {
            gfx->CmdSetViewportAuto();
      }
      entt::entity current_kernel_instance = entt::null;
      for (auto& cmd : _commands) {
            switch (cmd.Command) {
                  case PtxRenderNodeCommand::DRAW:
                        if (current_kernel_instance != pfx->_kernelNormalInstance) {
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
      if (!_commands.empty() && viewport.has_value()) {
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

void RenderNode::DrawRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
      //pos, uv, color
      const std::array<PtxRenderNodeVertexPack, 4> square_vt = {
            MapPos(x, y + height, 0, 0, r, g, b, a),
            MapPos(x + width, y + height, 0, 0, r, g, b, a),
            MapPos(x , y, 0, 0, r, g, b, a),
            MapPos(x + width, y, 0, 0, r, g, b, a),
      };

      const auto current_vertex_count = (uint32_t)_vertexList.size();
      const auto current_index_count = (uint32_t)_indexList.size();

      const std::array square_id = {
            0 + current_vertex_count, 1 + current_vertex_count, 2 + current_vertex_count,
            3 + current_vertex_count, 2 + current_vertex_count, 1+ current_vertex_count
      };

      _vertexList.resize(_vertexList.size() + square_vt.size());
      _indexList.resize(_indexList.size() + square_id.size());

      memcpy(&_vertexList.at(current_vertex_count), square_vt.data(), square_vt.size() * sizeof(PtxRenderNodeVertexPack));
      memcpy(&_indexList.at(current_index_count), square_id.data(), square_id.size() * sizeof(uint32_t));

      if (_commands.empty() || _commands.back().Command != PtxRenderNodeCommand::DRAW) {
            _commands.emplace_back(PtxRenderNodeCommand::DRAW);
            _commands.back().DrawRange.StartIndex = current_index_count;
      }

      _commands.back().DrawRange.EndIndex = static_cast<uint32_t>(_indexList.size());
}

void RenderNode::NewLayer() {
      if (!_commands.empty() && _commands.back().Command != PtxRenderNodeCommand::DRAW) {
            _commands.emplace_back(PtxRenderNodeCommand::NEW_LAYER);
      }
}

void RenderNode::GetPathSegmentLineTangent(std::span<glm::ivec2> pos, bool closed, std::vector<glm::vec2>& tang) {
      if(pos.size() < 2) return;
      if(closed && pos.size() > 2) {
            tang.resize(pos.size());

            uint32_t simd_loop = (tang.size() - 1) / 4;
            uint32_t i = 0;
            for(i = 0; i < simd_loop * 4; i += 4) {
                  glm::vec2 temp1 = pos[i + 1] - pos[i];
                  glm::vec2 temp2 = pos[i + 2] - pos[i + 1];
                  glm::vec2 temp3 = pos[i + 3] - pos[i + 2];
                  glm::vec2 temp4 = pos[i + 4] - pos[i + 3];

                  tang[i] = glm::normalize(temp1);
                  tang[i+ 1] = glm::normalize(temp2);
                  tang[i+ 2] = glm::normalize(temp3);
                  tang[i+ 3] = glm::normalize(temp4);
                  // printf("STans (%f, %f)\n", tang[i].x, tang[i].y);
                  // printf("STans (%f, %f)\n", tang[i + 1].x, tang[i + 1].y);
                  // printf("STans (%f, %f)\n", tang[i + 2].x, tang[i + 2].y);
                  // printf("STans (%f, %f)\n", tang[i + 3].x, tang[i + 3].y);
            }

            for(; i < tang.size() - 1; i++) {
                  glm::vec2 temp = pos[i + 1] - pos[i];
                  tang[i] = glm::normalize(temp);
                  //printf("STans (%f, %f)\n", tang[i].x, tang[i].y);
            }
            glm::vec2 temp = pos.front() - pos.back();
            tang.back() = glm::normalize(glm::vec2(pos.front() - pos.back()));
            //printf("STans (%f, %f)\n", tang.back().x, tang.back().y);

      } else {
            tang.resize(pos.size() - 1);

            uint32_t simd_loop = (tang.size() - 1) / 4;
            uint32_t i = 0;
            for(i = 0; i < simd_loop * 4; i += 4) {
                  glm::vec2 temp1 = pos[i + 1] - pos[i];
                  glm::vec2 temp2 = pos[i + 2] - pos[i + 1];
                  glm::vec2 temp3 = pos[i + 3] - pos[i + 2];
                  glm::vec2 temp4 = pos[i + 4] - pos[i + 3];
                  tang[i] = glm::normalize(temp1);
                  tang[i+ 1] = glm::normalize(temp2);
                  tang[i+ 2] = glm::normalize(temp3);
                  tang[i+ 3] = glm::normalize(temp4);
                  //printf("STans (%f, %f)\n", tang[i].x, tang[i].y);
            }

            for(i = 0; i < tang.size(); i++) {
                  tang[i] = glm::normalize(glm::vec2(pos[i + 1] - pos[i]));
                  //printf("STans (%f, %f)\n", tang[i].x, tang[i].y);
            }
      }
}

void RenderNode::GetPathSegmentLineNormal(std::span<glm::ivec2> pos, bool closed, std::vector<glm::vec2>& normal, std::vector<glm::vec2>& tang, std::vector<bool> &sharp_angle) {
      if(pos.size() < 2) return;
      GetPathSegmentLineTangent(pos, closed, tang);

      normal.resize(tang.size());
      sharp_angle.resize(tang.size() - 1);

      uint32_t simd_count = normal.size() / 4;
      uint32_t i = 0;

      for(; i < simd_count; i+= 4) {
            normal[i] = glm::vec2(-tang[i].y, tang[i].x);
            normal[i + 1] = glm::vec2(-tang[i + 1].y, tang[i + 1].x);
            normal[i + 2] = glm::vec2(-tang[i + 2].y, tang[i + 2].x);
            normal[i + 3] = glm::vec2(-tang[i + 3].y, tang[i + 3].x);
      }

      for(; i < normal.size(); i++) {
            normal[i] = glm::vec2(-tang[i].y, tang[i].x);
      }

      simd_count = (normal.size() - 1) / 4;

      for(i = 0; i < simd_count; i += 4) {
            sharp_angle[i ] = glm::cross(glm::vec3(tang[i + 1], 0), glm::vec3(-tang[i], 0)).z > 0;
            sharp_angle[i + 1] = glm::cross(glm::vec3(tang[i + 2], 0), glm::vec3(-tang[i + 1], 0)).z > 0;
            sharp_angle[i + 2] = glm::cross(glm::vec3(tang[i + 3], 0), glm::vec3(-tang[i + 2], 0)).z > 0;
            sharp_angle[i + 3] = glm::cross(glm::vec3(tang[i + 4], 0), glm::vec3(-tang[i + 3], 0)).z > 0;
      }

      for(; i < normal.size() - 1; i++) {
            glm::vec3 prev = glm::vec3(-tang[i], 0);
            glm::vec3 current = glm::vec3(tang[i + 1], 0);
            glm::vec3 C = glm::cross(current, prev);
            sharp_angle[i] = C.z > 0;
      }

      if(closed && pos.size() > 2) {
            glm::vec3 end_current = glm::vec3(tang.front(), 0);
            glm::vec3 end_prev = glm::vec3(-tang.back(), 0);
            glm::vec3 C = glm::cross(end_current, end_prev);
            sharp_angle.emplace_back(C.z > 0);
      }

       // for(uint32_t i = 0; i < normal.size(); i++) {
       //       printf("Normal (%f, %f)\n", normal[i].x, normal[i].y);
       // }
}

void RenderNode::GetPathSegmentJoltPointNormal(std::span<glm::ivec2> pos, bool closed,
      std::vector<glm::vec2>& line_normal,
      std::vector<glm::vec2>& line_tang,
      std::vector<bool>& angle_sharp,
      std::vector<glm::vec2>& jolts_normal,
      std::vector<float>& half_angle) {

      if(pos.size() < 2) return;
      GetPathSegmentLineNormal(pos, closed, line_normal, line_tang, angle_sharp);
      if(pos.size() == 2) return;

      if(closed) { // 封闭
            jolts_normal.resize(line_normal.size());
            half_angle.resize(line_normal.size());

            uint32_t simd_count = (line_normal.size() - 1) / 4;
            uint32_t i = 0;
            for(; i < simd_count; i += 4) {
                  jolts_normal[i] = glm::normalize(line_normal[i] + line_normal[i + 1]);
                  jolts_normal[i + 1] = glm::normalize(line_normal[i + 1] + line_normal[i + 2]);
                  jolts_normal[i + 2] = glm::normalize(line_normal[i + 2] + line_normal[i + 3]);
                  jolts_normal[i + 3] = glm::normalize(line_normal[i + 3] + line_normal[i + 4]);

                  half_angle[i] = (glm::pi<float>() - glm::acos(glm::dot(line_normal[i], line_normal[i + 1]))) / 2.0f;
                  half_angle[i + 1] = (glm::pi<float>() - glm::acos(glm::dot(line_normal[i + 1], line_normal[i + 2]))) / 2.0f;
                  half_angle[i + 2] = (glm::pi<float>() - glm::acos(glm::dot(line_normal[i + 2], line_normal[i + 3]))) / 2.0f;
                  half_angle[i + 3] = (glm::pi<float>() - glm::acos(glm::dot(line_normal[i + 3], line_normal[i + 4]))) / 2.0f;
                  //printf("Jolt Normal (%f, %f) Sharp angel %f\n",jolts_normal[i].x, jolts_normal[i].y,  glm::degrees(half_angle[i]));
                  //printf("Jolt Normal (%f, %f) Sharp angel %f\n",jolts_normal[i + 1].x, jolts_normal[i + 1].y,  glm::degrees(half_angle[i]));
                  //printf("Jolt Normal (%f, %f) Sharp angel %f\n",jolts_normal[i + 2].x, jolts_normal[i + 2].y,  glm::degrees(half_angle[i]));
                  //printf("Jolt Normal (%f, %f) Sharp angel %f\n",jolts_normal[i + 3].x, jolts_normal[i + 3].y,  glm::degrees(half_angle[i]));
            }

            for(; i < line_normal.size() - 1; i++) {
                  jolts_normal[i] = glm::normalize(line_normal[i] + line_normal[i + 1]);
                  half_angle[i] = (glm::pi<float>() - glm::acos(glm::dot(line_normal[i], line_normal[i + 1]))) / 2.0f;
                  //printf("Jolt Normal (%f, %f) Sharp angel %f\n",jolts_normal[i].x, jolts_normal[i].y,  glm::degrees(half_angle[i]));
            }

            jolts_normal.back() = glm::normalize(line_normal.back() + line_normal.front());
            half_angle.back() = (glm::pi<float>() - glm::acos(glm::dot(line_normal.back(), line_normal.front()))) / 2.0f;
            //printf("Jolt Normal (%f, %f) Sharp angel %f\n",jolts_normal.back().x, jolts_normal.back().y,  glm::degrees(half_angle.back()));

      } else {
            jolts_normal.resize(line_normal.size() - 1);
            half_angle.resize(line_normal.size() - 1);

            uint32_t simd_count = (line_normal.size() - 1) / 4;
            uint32_t i = 0;

            for(; i < simd_count; i += 4) {
                  jolts_normal[i] = glm::normalize(line_normal[i] + line_normal[i + 1]);
                  jolts_normal[i + 1] = glm::normalize(line_normal[i + 1] + line_normal[i + 2]);
                  jolts_normal[i + 2] = glm::normalize(line_normal[i + 2] + line_normal[i + 3]);
                  jolts_normal[i + 3] = glm::normalize(line_normal[i + 3] + line_normal[i + 4]);

                  half_angle[i] = (glm::pi<float>() - glm::acos(glm::dot(line_normal[i], line_normal[i + 1]))) / 2.0f;
                  half_angle[i + 1] = (glm::pi<float>() - glm::acos(glm::dot(line_normal[i + 1], line_normal[i + 2]))) / 2.0f;
                  half_angle[i + 2] = (glm::pi<float>() - glm::acos(glm::dot(line_normal[i + 2], line_normal[i + 3]))) / 2.0f;
                  half_angle[i + 3] = (glm::pi<float>() - glm::acos(glm::dot(line_normal[i + 3], line_normal[i + 4]))) / 2.0f;
            }

            for(; i < line_normal.size() - 1; i++) {
                  jolts_normal[i] = glm::normalize(line_normal[i] + line_normal[i + 1]);
                  half_angle[i] = (glm::pi<float>() - glm::acos(glm::dot(line_normal[i], line_normal[i + 1]))) / 2.0f;
                  //printf("Jolt Normal (%f, %f) Sharp angel %f\n",jolts_normal[i].x, jolts_normal[i].y,  glm::degrees(half_angle[i]));
            }
      }
}

void RenderNode::ExtrudeUp(std::span<glm::ivec2> pos, bool closed, uint32_t distance, uint32_t expend_layers, std::vector<glm::vec2>& vertex, std::vector<uint32_t>& index) {
      if(pos.size() <= 2) { return; }

      GetPathSegmentJoltPointNormal(pos, closed, line_norm, line_tang, angle_sharp, jolt_norm, half_angle);

      if(closed && pos.size() > 2) { // 封闭

            vertex.resize(line_tang.size() * 4);
            index.resize(line_tang.size() * 12);

            //first
            uint32_t last_t {};
            uint32_t last_m {};
            uint32_t last_b {};

            {
                  vertex[1] = pos.front(); // mid left

                  if(angle_sharp.back()) {
                        vertex[0] = (glm::vec2(pos.front()) + jolt_norm.back() * ((float)distance / glm::sin(half_angle.back()))); // top
                        vertex[2] = (glm::vec2(pos.front()) - line_norm.front() * (float)distance); // bottom
                        vertex.back() = (glm::vec2(pos.front()) - line_norm.back() * (float)distance);  // bottom last

                        last_t = 0;
                        last_m = 1;
                        last_b= vertex.size() - 1;

                  } else {
                        vertex[0] = (glm::vec2(pos.front()) + line_norm.front() * (float)distance); // top
                        vertex.back() = (glm::vec2(pos.front()) + line_norm.back() * (float)distance); // top last

                        vertex[2] = (glm::vec2(pos.front()) - jolt_norm.back() * ((float)distance / glm::sin(half_angle.back()))); // bottom

                        last_t = vertex.size() - 1;
                        last_m = 1;
                        last_b= 2;
                  }
            }

            uint32_t t = 0, m = 1 ,b = 2;
            for (uint32_t i  = 0; i < line_norm.size() - 1; i++) {

                  const uint32_t idx = 3 + i * 4;
                  const uint32_t index_offset = i * 12;

                  index[index_offset] = t;
                  index[index_offset + 1] = idx + 1;
                  index[index_offset + 2] = m;

                  index[index_offset + 3] = idx;
                  index[index_offset + 4] = m;
                  index[index_offset + 5] = idx + 1;

                  index[index_offset + 6] = m;
                  index[index_offset + 7] = idx;
                  index[index_offset + 8] = b;

                  index[index_offset + 9] = idx + 2;
                  index[index_offset + 10] = b;
                  index[index_offset + 11] = idx;

                  vertex[idx] = pos[i + 1]; // mid

                  if(angle_sharp[i]) {
                        vertex[idx + 1] = (glm::vec2(pos[i + 1]) + jolt_norm[i] * ((float)distance / glm::sin(half_angle[i]))); // top

                        vertex[idx + 2] = (glm::vec2(pos[i + 1]) - line_norm[i] * (float)distance); // bottom
                        vertex[idx + 3] = (glm::vec2(pos[i + 1]) - line_norm[i + 1] * (float)distance); // bottom next

                        m = idx;
                        t = idx + 1;
                        b = idx + 3;
                  } else {
                        vertex[idx + 1] = (glm::vec2(pos[i + 1]) + line_norm[i] * (float)distance); // top
                        vertex[idx + 3] = (glm::vec2(pos[i + 1]) + line_norm[i + 1] * (float)distance); // top next

                        vertex[idx + 2] = (glm::vec2(pos[i + 1]) - jolt_norm[i] * ((float)distance / glm::sin(half_angle[i]))); // bottom

                        m = idx;
                        t = idx + 3;
                        b = idx + 2;
                  }
            }

            {
                  const uint32_t index_offset = (line_norm.size() - 1) * 12;

                  index[index_offset] = t;
                  index[index_offset + 1] = last_t;
                  index[index_offset + 2] = m;

                  index[index_offset + 3] = last_m;
                  index[index_offset + 4] = m;
                  index[index_offset + 5] = last_t;

                  index[index_offset + 6] = m;
                  index[index_offset + 7] = last_m;
                  index[index_offset + 8] = b;

                  index[index_offset + 9] = last_b;
                  index[index_offset + 10] = b;
                  index[index_offset + 11] = last_m;
            }


      } else {

            vertex.resize((line_tang.size()- 1) * 4 + 6);
            index.resize(line_tang.size() * 12);
            //first

            {
                  vertex[0] = glm::vec2(pos[0]) + line_norm[0] * (float)distance; // top
                  vertex[1] = pos[0]; // mid
                  vertex[2] = glm::vec2(pos[0]) - line_norm[0] * (float)distance; // bottom
            }

            uint32_t t = 0, m = 1 ,b = 2;
            for (uint32_t i  = 0; i < line_norm.size() - 1; i++) {

                  const uint32_t idx = 3 + i * 4;
                  const uint32_t index_offset = i * 12;

                  index[index_offset] = t;
                  index[index_offset + 1] = idx + 1;
                  index[index_offset + 2] = m;

                  index[index_offset + 3] = idx;
                  index[index_offset + 4] = m;
                  index[index_offset + 5] = idx + 1;

                  index[index_offset + 6] = m;
                  index[index_offset + 7] = idx;
                  index[index_offset + 8] = b;

                  index[index_offset + 9] = idx + 2;
                  index[index_offset + 10] = b;
                  index[index_offset + 11] = idx;

                  vertex[idx] = pos[i + 1]; // mid

                  if(angle_sharp[i]) {
                        vertex[idx + 1] = (glm::vec2(pos[i + 1]) + jolt_norm[i] * ((float)distance / glm::sin(half_angle[i]))); // top

                        vertex[idx + 2] = (glm::vec2(pos[i + 1]) - line_norm[i] * (float)distance); // bottom
                        vertex[idx + 3] = (glm::vec2(pos[i + 1]) - line_norm[i + 1] * (float)distance); // bottom next

                        m = idx;
                        t = idx + 1;
                        b = idx + 3;
                  } else {
                        vertex[idx + 1] = (glm::vec2(pos[i + 1]) + line_norm[i] * (float)distance); // top
                        vertex[idx + 3] = (glm::vec2(pos[i + 1]) + line_norm[i + 1] * (float)distance); // top next

                        vertex[idx + 2] = (glm::vec2(pos[i + 1]) - jolt_norm[i] * ((float)distance / glm::sin(half_angle[i]))); // bottom

                        m = idx;
                        t = idx + 3;
                        b = idx + 2;
                  }
            }

            {
                  vertex[vertex.size() - 3] = glm::vec2(pos.back()) + line_norm.back() * (float)distance; // top
                  vertex[vertex.size() - 2] = pos.back(); // mid
                  vertex[vertex.size() - 1] = glm::vec2(pos.back()) - line_norm.back() * (float)distance; // bottom

                  const uint32_t index_offset = (line_norm.size() - 1) * 12;

                  index[index_offset] = t;
                  index[index_offset + 1] = vertex.size() - 3;
                  index[index_offset + 2] = m;

                  index[index_offset + 3] = vertex.size() - 2;
                  index[index_offset + 4] = m;
                  index[index_offset + 5] = vertex.size() - 3;

                  index[index_offset + 6] = m;
                  index[index_offset + 7] = vertex.size() - 2;
                  index[index_offset + 8] = b;

                  index[index_offset + 9] = vertex.size() - 1;
                  index[index_offset + 10] = b;
                  index[index_offset + 11] = vertex.size() - 2;
            }
      }
}

void RenderNode::DrawPath(std::span<glm::ivec2> pos, bool closed, uint32_t thickness, uint32_t r, uint32_t g, uint32_t b, uint32_t a, uint32_t expend) {
      if(pos.size() < 2) {
            const auto error = "Path must have at least 2 points";
            MessageManager::Log(MessageType::Error, error);
            throw std::runtime_error(error);
      }

      if(pos.size() != 2) {

            ExtrudeUp(pos, closed, thickness, 1, expended_vertex, indexs);

            const auto current_index_count = (uint32_t)_indexList.size();
            const auto current_vertex_count = (uint32_t)_vertexList.size();

            _vertexList.reserve(_vertexList.size() + (expended_vertex.size() * sizeof(glm::vec2)));
            for (size_t i = 0; i < expended_vertex.size(); i++) {
                  _vertexList.emplace_back(MapPos(expended_vertex[i], {0.0, 0.0}, {r,b,b,a}));
            }

            _indexList.reserve(_indexList.size() + indexs.size());
            for (size_t i = 0; i < indexs.size(); i++) {
                  _indexList.emplace_back(indexs[i] + current_vertex_count);
            }

            if (_commands.empty() || _commands.back().Command != PtxRenderNodeCommand::DRAW) {
                  _commands.emplace_back(PtxRenderNodeCommand::DRAW);
                  _commands.back().DrawRange.StartIndex = current_index_count;
            }

            _commands.back().DrawRange.EndIndex = static_cast<uint32_t>(_indexList.size());
      } else {

      }
}

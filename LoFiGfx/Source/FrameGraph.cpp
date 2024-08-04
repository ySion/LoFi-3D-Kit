//
// Created by Arzuo on 2024/7/8.
//
#include "FrameGraph.h"
#include "Message.h"
#include "GfxContext.h"
#include "GfxComponents/Texture.h"

using namespace LoFi::Internal;
using namespace LoFi;

FrameGraph::~FrameGraph() {
      printf("End");
}

FrameGraph::FrameGraph(const std::array<VkCommandBuffer, 3>& cmdbuffers) : _cmdBuffer(cmdbuffers), _world(*volkGetLoadedEcsWorld()) {}

bool FrameGraph::CheckNodeExist(const std::string& name) const {
      return _nodeMap.contains(name);
}

bool FrameGraph::AddNode(RenderNode* node) {
      if (_nodeMap.contains(node->GetNodeName())) {
            const auto err = std::format("[FrameGraph::AddNode] Node Name \"{}\" already exist.", node->GetNodeName());
            MessageManager::Log(MessageType::Error, err);
            return false;
      }

      _nodeMap[node->GetNodeName()] = node;
      _isGraphChanged = true;
      node->SetFrameIndex(_frameIndex);
      node->PrepareFrame();
      return true;
}

void FrameGraph::RemoveNode(RenderNode* node) {
      if (_nodeMap.contains(node->GetNodeName())) {
            _nodeMap.erase(node->GetNodeName());
            _isGraphChanged = true;
      }
}

void FrameGraph::SetNeedUpdate() {
      _isGraphChanged = true;
}

void FrameGraph::SetRootNode(ResourceHandle node) {
      if (node.Type == GfxEnumResourceType::RenderGraphNode) {
            const RenderNode* ptr = GfxContext::Get()->ResourceFetch<RenderNode>(node);
            if (!ptr) {
                  const auto err = std::format("[FrameGraph::SetRootNode] Invalid RenderGraphNode handle.");
                  MessageManager::Log(MessageType::Error, err);
                  return;
            }
            _isGraphChanged = true;
            _rootNode = node;
      } else {
            const auto err = std::format("[FrameGraph::SetRootNode] Invalid Resource Type, Need a RenderGraphNode, but got {}.", ToStringResourceType(node.Type));
            MessageManager::Log(MessageType::Error, err);
            return;
      }
}

void FrameGraph::GenFrameGraph(uint32_t frame_index) {
      _frameIndex = frame_index;

      if(_nodeMap.empty()) return;
      if (_rootNode.Type != GfxEnumResourceType::RenderGraphNode) {
            const auto err = "[FrameGraph::GenFrameGraph] RootNode is not set, Please set the root node first, nothing generate.";
            MessageManager::Log(MessageType::Error, err);
            return;
      }
      RenderNode* ptr = _world.try_get<RenderNode>(_rootNode.RHandle);
      if (!ptr) {
            const auto err = std::format("[FrameGraph::GenFrameGraph] RootNode takes a Invalid RenderGraphNode handle.");
            MessageManager::Log(MessageType::Error, err);
            return;
      }

      _ptrRootNode = ptr;
      if (_isGraphChanged) {
            auto view = _world.view<RenderNode>();
            view.each([&](entt::entity id, RenderNode& node) {
                  node._nodeAfter.clear();
            });

            view.each([&](entt::entity id, RenderNode& node) {
                  for (const auto& wait_node : node._waitNodes) {
                        if (auto find = _nodeMap.find(wait_node); find == _nodeMap.end()) {
                              const auto err = std::format(R"([FrameGraph::GenFrameGraph] Invalid Node Link "{}" -> "{}", Node "{}" not exist!)",
                              wait_node, node.GetNodeName(), wait_node);
                              MessageManager::Log(MessageType::Error, err);
                              break;
                        } else {
                              find->second->_nodeAfter.push_back(&node);
                        }
                  }
            });


            while (!_nodeCheckStack.empty()) {
                  _nodeCheckStack.pop();
            }

            _nodeMapCycleCheckCache.clear();
            if (IsGraphCycle(_ptrRootNode)) {
                  std::string cycle_print{};
                  while (!_nodeCheckStack.empty()) {
                        if (_nodeCheckStack.top()->GetNodeName() == _nodeCheckRepeat) {
                              cycle_print += std::format("({}) -> ", _nodeCheckRepeat);
                        } else {
                              cycle_print += std::format("({}) -> ", _nodeCheckStack.top()->GetNodeName());
                        }
                        _nodeCheckStack.pop();
                  }
                  cycle_print += "..";
                  const auto err = std::format("[FrameGraph::GenFrameGraph] RenderGraph has cycle, Please check the nodes' RelationShip:\n {}", cycle_print);
                  MessageManager::Log(MessageType::Error, err);
                  return;
            }

            //Gen sort
            _nodeList.clear();
            _nodeMapCycleCheckCache.clear();

            _nodeList.push_back(_ptrRootNode);
            _nodeMapCycleCheckCache.insert(_ptrRootNode);
            GraphSort(_ptrRootNode);

            //print
            for (const auto i : _nodeList) {
                  printf("%s -> ", i->GetNodeName().c_str());
            }
            printf("$End\n");

            printf("All Nodes:");
            for (const auto i : _nodeMap) {
                  printf("%s,  ", i.first.c_str());
            }
            printf("\n");
      }

      _resourceFinalBarrierBuffer.clear();
      _resourceFinalBarrierTexture.clear();

      RenderNode* prev_node = nullptr;
      VkCommandBuffer current_buf = _cmdBuffer[_frameIndex];
      for (const auto& node : _nodeList) {
            if (!prev_node) {
                  for (const auto& i : node->_beginBarrierBuffer) {
                        i.buffer->BarrierLayout(current_buf, i.first_barrier_kernel_type, i.first_barrier_usage);
                        _resourceFinalBarrierBuffer[i.buffer] = {i.first_barrier_kernel_type, i.first_barrier_usage};
                  }
                  for (const auto& i : node->_beginBarrierTexture) {
                        i.texture->BarrierLayout(current_buf, i.first_barrier_kernel_type, i.first_barrier_usage);
                        _resourceFinalBarrierTexture[i.texture] = {i.first_barrier_kernel_type, i.first_barrier_usage};
                  }
            } else {
                  for (const auto& i : node->_beginBarrierBuffer) {
                        if (auto find_prev = prev_node->_barrierTableBuffer.find(i.buffer); find_prev != prev_node->_barrierTableBuffer.end()) {
                              RenderNode::CmdBarrierBuffer(current_buf, i.buffer, find_prev->second.kernel_type,
                              find_prev->second.usage, i.first_barrier_kernel_type, i.first_barrier_usage, node->GetNodeName());
                              _resourceFinalBarrierBuffer[i.buffer] = {i.first_barrier_kernel_type, i.first_barrier_usage};
                        } else {
                              i.buffer->BarrierLayout(current_buf, i.first_barrier_kernel_type, i.first_barrier_usage);
                              _resourceFinalBarrierBuffer[i.buffer] = {i.first_barrier_kernel_type, i.first_barrier_usage};
                        }
                  }
                  for (const auto& i : node->_beginBarrierTexture) {
                        if (auto find_prev = prev_node->_barrierTableTexture.find(i.texture); find_prev != prev_node->_barrierTableTexture.end()) {
                              RenderNode::CmdBarrierTexture(current_buf, i.texture, find_prev->second.kernel_type,
                              find_prev->second.usage, i.first_barrier_kernel_type, i.first_barrier_usage, node->GetNodeName());
                              _resourceFinalBarrierTexture[i.texture] = {i.first_barrier_kernel_type, i.first_barrier_usage};
                        } else {
                              i.texture->BarrierLayout(current_buf, i.first_barrier_kernel_type, i.first_barrier_usage);
                              _resourceFinalBarrierTexture[i.texture] = {i.first_barrier_kernel_type, i.first_barrier_usage};
                        }
                  }
            }
            node->EmitCommands(current_buf);
            prev_node = node;
      }

      //final Barrier
      for (const auto i : _resourceFinalBarrierBuffer) {
            i.first->SetLayout(i.second.first, i.second.second);
      }

      for (const auto i : _resourceFinalBarrierTexture) {
            i.first->SetLayout(i.second.first, i.second.second);
      }

      _isGraphChanged = false;
}

void FrameGraph::PrepareNextFrame() const {
      _world.view<RenderNode>().each([&](entt::entity id, RenderNode& node) {
            node.SetFrameIndex(_frameIndex + 1);
            node.PrepareFrame();
      });
}

bool FrameGraph::IsGraphCycle(RenderNode* node) {
      _nodeCheckStack.push(node);
      if (_nodeMapCycleCheckCache.contains(node)) {
            //cycling
            _nodeCheckRepeat = node->GetNodeName();
            return true;
      } else {
            _nodeMapCycleCheckCache.insert(node);
      }

      for (const auto i : node->_nodeAfter) {
            if (IsGraphCycle(i)) {
                  return true;
            }
      }
      _nodeMapCycleCheckCache.erase(node);
      _nodeCheckStack.pop();
      return false;
}

void FrameGraph::GraphSort(RenderNode* node) {
      for (const auto i : node->_nodeAfter) {
            if (!_nodeMapCycleCheckCache.contains(i)) {
                  _nodeList.push_back(i);
                  _nodeMapCycleCheckCache.insert(i);
            }
      }

      for (const auto i : node->_nodeAfter) {
            GraphSort(i);
      }
}

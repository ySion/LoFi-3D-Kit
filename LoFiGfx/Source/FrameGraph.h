//
// Created by Arzuo on 2024/7/8.
//

#pragma once
#include <stack>

#include "Helper.h"
#include "RenderNode.h"

namespace LoFi {

    class FrameGraph {
    public:

        NO_COPY_MOVE_CONS(FrameGraph);

        ~FrameGraph();

        explicit FrameGraph(const std::array<VkCommandBuffer, 3>& cmdbuffers);

    public:

        bool AddNode(RenderNode *node);

        void RemoveNode(RenderNode* node);

        void SetNeedUpdate();


        bool CheckNodeExist(const std::string& name) const;

        void SetRootNode(ResourceHandle node);

        void GenFrameGraph(uint32_t frame_index);

        void PrepareNextFrame() const;

    public:

        bool IsGraphCycle(RenderNode* node);

        void GraphSort(RenderNode* node);

    private:

        ResourceHandle _rootNode = {};

        RenderNode* _ptrRootNode = nullptr;

        std::vector<RenderNode*> _nodeList{};

        entt::dense_map<std::string, RenderNode*> _nodeMap{};

        entt::dense_set<RenderNode*> _nodeMapCycleCheckCache{};

        std::stack<RenderNode*> _nodeCheckStack{};

        std::string _nodeCheckRepeat{};

        entt::dense_map<Component::Gfx::Texture*, std::pair<GfxEnumKernelType, GfxEnumResourceUsage>> _resourceFinalBarrierTexture{};

        entt::dense_map<Component::Gfx::Buffer*, std::pair<GfxEnumKernelType, GfxEnumResourceUsage>> _resourceFinalBarrierBuffer{};

    private:

        std::array<VkCommandBuffer, 3> _cmdBuffer{};

        entt::registry& _world;

        friend class GfxContext;
    private:

        bool _isGraphChanged = true; // 依赖关系改变, 以及节点变多, 比如调用After, 以及插入新节点的时候, 需要重新排序 // TODO


        uint32_t _frameIndex = 0;
    };

}

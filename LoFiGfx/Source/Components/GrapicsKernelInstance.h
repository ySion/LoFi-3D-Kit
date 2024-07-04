//
// Created by Arzuo on 2024/6/28.
//

#pragma once

#include "GraphicKernel.h"

namespace LoFi::Component {

      class GrapicsKernelInstance {
      public:
            NO_COPY_MOVE_CONS(GrapicsKernelInstance);

            ~GrapicsKernelInstance();

            //if param_high_dynamic is true, param buffer will emplace with host access, it might be slow to read for gpu;
            explicit GrapicsKernelInstance(entt::entity id, entt::entity graphics_kernel, bool param_high_dynamic = true);

            [[nodiscard]] entt::entity GetHandle() const { return _id; }

            [[nodiscard]] bool IsParamHighDynamic() const { return _isParamHighDynamic; }

            [[nodiscard]] entt::entity GetParentGraphicsKernel() const { return _parent; }

            bool SetParam(const std::string& param_struct_name, const void* data); // likes "Info"

            bool SetParamMember(const std::string& param_struct_member_name, const void* data); // likes "Info.time"

            bool SetSampled(const std::string& sampled_name, entt::entity texture);

      private:
            friend class Context;

            void PushResourceChanged();

            void PushBindlessInfo(VkCommandBuffer buf) const;

            entt::entity _id;

            entt::entity _parent; // Graphics kernel or FrameResource

            bool _isParamHighDynamic;

            std::vector<KernelParamResource> _paramBuffers{};

            std::vector<uint32_t> _pushConstantBindlessIndexInfoBuffer{}; // BindlessInfo

            std::vector<std::optional<std::pair<entt::entity, ProgramShaderReourceType>>> _resourceBind{};
      };
}

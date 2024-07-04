//
// Created by Arzuo on 2024/7/4.
//
#pragma once

#include "ComputeKernel.h"

namespace LoFi::Component {
      class ComputeKernelInstance {
      public:
            NO_COPY_MOVE_CONS(ComputeKernelInstance);

            //if param_high_dynamic is true, param buffer will emplace with host access, it might be slow to read for gpu;
            explicit ComputeKernelInstance(entt::entity id, entt::entity compute_kernel, bool param_high_dynamic = true);

            ~ComputeKernelInstance();

            [[nodiscard]] entt::entity GetHandle() const { return _id; }

            [[nodiscard]] bool IsParamHighDynamic() const { return _isParamHighDynamic; }

            [[nodiscard]] entt::entity GetParentGraphicsKernel() const { return _parent; }

            bool SetParam(const std::string& struct_name, const void* data);

            bool SetParamMember(const std::string& struct_member_name, const void* data);

            bool SetSampled(const std::string& sampled_name, entt::entity texture);

            bool SetTexture(const std::string& sampled_name, entt::entity texture);

            bool SetBuffer(const std::string& sampled_name, entt::entity buffer);

      private:
            friend class Context;

            void PushResourceChanged();

            void PushBindlessInfo(VkCommandBuffer buf) const;

            void ResourceBarrierPrepare(VkCommandBuffer buf) const;

            entt::entity _id;

            entt::entity _parent; // Graphics kernel or FrameResource

            bool _isParamHighDynamic;

            std::vector<KernelParamResource> _paramBuffers{};

            std::vector<uint32_t> _pushConstantBindlessIndexInfoBuffer{}; // BindlessInfo

            std::vector<std::optional<std::pair<entt::entity, ProgramShaderReourceType>>> _resourceBind{};
      };
}

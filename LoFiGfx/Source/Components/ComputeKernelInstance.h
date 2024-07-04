//
// Created by Arzuo on 2024/7/4.
//
#pragma once

#include "ComputeKernel.h"

namespace LoFi::Component {
      class ComputeKernelInstance {
      public:
            NO_COPY_MOVE_CONS(ComputeKernelInstance);

            explicit ComputeKernelInstance(entt::entity id, entt::entity compute_kernel, bool is_cpu_side = true);

            ~ComputeKernelInstance();

            [[nodiscard]] entt::entity GetHandle() const { return _id; }

            [[nodiscard]] bool IsCpuSide() const { return _isCpuSide; }

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

            entt::entity _id;

            entt::entity _parent; // Graphics kernel or FrameResource

            bool _isCpuSide;

            std::vector<KernelParamResource> _buffers{};

            std::vector<uint32_t> _pushConstantBindlessIndexInfoBuffer{}; // BindlessInfo
      };
}

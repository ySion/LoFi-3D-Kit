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

            explicit GrapicsKernelInstance(entt::entity id, entt::entity graphics_kernel, bool is_cpu_side = true);

            [[nodiscard]] entt::entity GetHandle() const { return _id; }

            [[nodiscard]] bool IsCpuSide() const { return _isCpuSide; }

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

            bool _isCpuSide;

            std::vector<KernelParamResource> _buffers{};

            std::vector<uint32_t> _pushConstantBindlessIndexInfoBuffer{}; // BindlessInfo
      };
}

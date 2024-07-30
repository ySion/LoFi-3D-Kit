//
// Created by Arzuo on 2024/7/7.
//

#pragma once

#include "Defines.h"

#include "../Helper.h"

namespace LoFi::Component::Gfx {
      class PushConstantBuffer {
      public:
            NO_COPY_MOVE_CONS(PushConstantBuffer);

            ~PushConstantBuffer();

            explicit PushConstantBuffer(entt::entity id, entt::entity kernel);

            [[nodiscard]] entt::entity GetHandle() const { return _id; }

            [[nodiscard]] entt::entity GetKernel() const { return _kernel; }

            void SetValue(const std::string& name, const void* data);

            void CmdPushConstants(VkCommandBuffer cmd) const;

      private:
            entt::entity _id;

            entt::entity _kernel;

            VkPipelineLayout _pipelineLayout{};

            const entt::dense_map<std::string, PushConstantMemberInfo>* _pushConstantDefine{};

            std::vector<uint8_t> _pushConstantBuffer{};
      };
}

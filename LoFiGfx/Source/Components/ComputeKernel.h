//
// Created by Arzuo on 2024/6/30.
//

#pragma once

#include "Kernel.h"
#include "Program.h"

namespace LoFi {
      class Context;
}

namespace LoFi::Component {
      class ComputeKernel {

      public:
            NO_COPY_MOVE_CONS(ComputeKernel);

            explicit ComputeKernel(entt::entity id, entt::entity program);

            ~ComputeKernel();

            [[nodiscard]] VkPipeline GetPipeline() const { return _pipeline; }

            [[nodiscard]] VkPipelineLayout GetPipelineLayout() const { return _pipelineLayout; }

            [[nodiscard]] VkPipelineLayout* GetPipelineLayoutPtr() { return &_pipelineLayout; }


            [[nodiscard]] const auto& GetParamTable() const { return _paramTable; }

            [[nodiscard]] const auto& GetParamMemberTable() const { return _paramMemberTable; }

            [[nodiscard]] const auto& GetReourceDefineTable() const { return _resourceDefineTable; }

            [[nodiscard]] const VkPushConstantRange& GetBindlessInfoPushConstantRange() const { return _pushConstantRange; }

      private:

            VkPipeline _pipeline{};

            VkPipelineLayout _pipelineLayout{};

            VkPushConstantRange _pushConstantRange{};

            entt::dense_map<std::string, ProgramParamInfo> _paramTable{};

            entt::dense_map<std::string, ProgramParamMemberInfo> _paramMemberTable{};

            entt::dense_map<std::string, ProgramShaderReourceDefine> _resourceDefineTable{};
      };
}

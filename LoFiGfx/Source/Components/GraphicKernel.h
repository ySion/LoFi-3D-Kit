//
// Created by starr on 2024/6/23.
//

#pragma once

#include "Kernel.h"
#include "Program.h"

namespace LoFi {
      class Context;
}

namespace LoFi::Component {

      class GraphicKernel {
      public:
            NO_COPY_MOVE_CONS(GraphicKernel);

            explicit GraphicKernel(entt::entity id, entt::entity program);

            ~GraphicKernel();

            [[nodiscard]] VkPipeline GetPipeline() const { return _pipeline; }

            [[nodiscard]] VkPipelineLayout GetPipelineLayout() const { return _pipelineLayout; }

            [[nodiscard]] VkPipelineLayout* GetPipelineLayoutPtr() { return &_pipelineLayout; }


            [[nodiscard]] const auto& GetParamTable() const { return _paramTable; }

            [[nodiscard]] const auto& GetParamMemberTable() const { return _paramMemberTable; }

            [[nodiscard]] const auto& GetReourceDefineTable() const { return _resourceDefineTable; }

            [[nodiscard]] const VkPushConstantRange& GetBindlessInfoPushConstantRange() const { return _pushConstantRange; }

      
      private:

            friend class ::LoFi::Context;
      private:
            entt::entity _id = entt::null;

            VkPipeline _pipeline{};

            VkPipelineLayout _pipelineLayout{};

            VkPushConstantRange _pushConstantRange{};

            entt::dense_map<std::string, ProgramParamInfo> _paramTable{};

            entt::dense_map<std::string, ProgramParamMemberInfo> _paramMemberTable{};

            entt::dense_map<std::string, ProgramShaderReourceDefine> _resourceDefineTable{};
      };
}

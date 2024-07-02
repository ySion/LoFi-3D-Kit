//
// Created by Arzuo on 2024/6/30.
//

#pragma once

#include "Program.h"
#include "../Helper.h"

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

            [[nodiscard]] const auto& GetSampledTextureTable() const { return _sampledTextureTable; }

            [[nodiscard]] const auto& GetBufferTable() const { return _bufferTable; }

            [[nodiscard]] const auto& GetTextureTable() const { return _textureTable; }

            [[nodiscard]] const auto& GetMarcoParserIdentifierTable() const {return _marcoParserIdentifier;}

            [[nodiscard]] const VkPushConstantRange& GetBindlessInfoPushConstantRange() const { return _pushConstantRange; }

      private:

            VkPipeline _pipeline{};

            VkPipelineLayout _pipelineLayout{};

            VkPushConstantRange _pushConstantRange{};

            entt::dense_map<std::string, ProgramParamInfo> _paramTable{};

            entt::dense_map<std::string, ProgramParamMemberInfo> _paramMemberTable{};

            entt::dense_map<std::string, uint32_t> _sampledTextureTable{};

            entt::dense_map<std::string, uint32_t> _bufferTable{};

            entt::dense_map<std::string, uint32_t> _textureTable{};

            std::vector<std::pair<std::string, std::string>> _marcoParserIdentifier{};


      };
}

//
// Created by starr on 2024/6/23.
//

#pragma once

#include "../Helper.h"

namespace LoFi {
      class Context;
}

namespace LoFi::Component {

      struct BindlessLayoutVariableInfo {
            uint32_t size;
            uint32_t offset;
      };

      class GraphicKernel {
      public:
            NO_COPY_MOVE_CONS(GraphicKernel);

            explicit GraphicKernel(entt::entity id);

            ~GraphicKernel();

            [[nodiscard]] VkPipeline GetPipeline() const { return _pipeline; }

            [[nodiscard]] VkPipelineLayout GetPipelineLayout() const { return _pipelineLayout; }

            [[nodiscard]] VkPipelineLayout* GetPipelineLayoutPtr() { return &_pipelineLayout; }

            [[nodiscard]] auto& GetBindlessLayoutVariableMap() const { return _pushConstantBufferVariableMap; }

            [[nodiscard]] std::optional<BindlessLayoutVariableInfo> SetBindlessLayoutVariable(const std::string& name) const;

            [[nodiscard]] bool SetBindlessLayoutVariable(const std::string& name, const void* data) const;

            void PushConstantBindlessLayoutVariableInfo(VkCommandBuffer cmd) const;

      private:
            bool CreateFromProgram(entt::entity program);

            friend class ::LoFi::Context;

      private:
            entt::entity _id = entt::null;

            VkPipeline _pipeline{};

            VkPipelineLayout _pipelineLayout{};

            entt::dense_map<std::string, BindlessLayoutVariableInfo> _pushConstantBufferVariableMap{};

            std::vector<uint8_t> _pushConstantBuffer{};
      };
}

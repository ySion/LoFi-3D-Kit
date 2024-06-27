//
// Created by starr on 2024/6/23.
//

#pragma once

#include "../Helper.h"

namespace LoFi {
      class Context;
}

namespace LoFi::Component {
      class GraphicKernel {
      public:
            NO_COPY_MOVE_CONS(GraphicKernel);

            explicit GraphicKernel(entt::entity id);

            ~GraphicKernel();

            bool CreateFromProgram(entt::entity program);

            [[nodiscard]] VkPipeline GetPipeline() const { return _pipeline; }

            [[nodiscard]] VkPipelineLayout GetPipelineLayout() const { return _pipelineLayout; }

            [[nodiscard]] VkPipelineLayout* GetPipelineLayoutPtr() { return &_pipelineLayout; }

      private:
            friend class ::LoFi::Context;

      private:
            entt::entity _id = entt::null;

            VkPipeline _pipeline{};

            VkPipelineLayout _pipelineLayout{};
      };
}

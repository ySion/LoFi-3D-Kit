//
// Created by Arzuo on 2024/6/30.
//

#pragma once

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

      private:

            VkPipeline _pipeline{};

            VkPipelineLayout _pipelineLayout{};

            VkPushConstantRange _pushConstantRange{};

      };
}

//
// Created by starr on 2024/6/23.
//

#pragma once

#include "../Helper.h"

namespace LoFi::Component {

      class GraphicKernel {
      public:

            NO_COPY_MOVE_CONS(GraphicKernel);

            explicit  GraphicKernel(entt::entity id);

            ~GraphicKernel();

            bool CreateFromProgram(entt::entity program) ;

      private:

            entt::entity _id = entt::null;

            VkPipeline _pipeline {};

            VkPipelineLayout _pipelineLayout {};
      };
}
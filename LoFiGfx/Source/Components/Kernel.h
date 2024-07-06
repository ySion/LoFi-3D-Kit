//
// Created by Arzuo on 2024/7/7.
//

#pragma once

#include "Defines.h"
#include "../Helper.h"

namespace LoFi::Component {
      class Program;
      class Kernel {
      public:
            NO_COPY_MOVE_CONS(Kernel);

            ~Kernel();

            explicit Kernel(entt::entity id, entt::entity program);

            [[nodiscard]] VkPipeline GetPipeline() const { return _pipeline; }

            [[nodiscard]] VkPipelineLayout GetPipelineLayout() const { return _pipelineLayout; }

            [[nodiscard]] VkPipelineLayout* GetPipelineLayoutPtr() { return &_pipelineLayout; }

            [[nodiscard]] bool IsComputeKernel() const { return _isComputeKernel; }

            [[nodiscard]] bool IsGraphicsKernel() const { return !_isComputeKernel; }

            [[nodiscard]] auto& GetPushConstantRange() const { return  _pushConstantRange;}

            [[nodiscard]] auto& GetPushConstantDefine() const { return _pushConstantDefine;  }

      private:
            void CreateAsGraphics(const Program* program);

            void CreateAsCompute(const Program* program);

      private:
            entt::entity _id = entt::null;

            bool _isComputeKernel = false;

            VkPipeline _pipeline{};

            VkPipelineLayout _pipelineLayout{};

            VkPushConstantRange _pushConstantRange{};

            entt::dense_map<std::string, PushConstantMemberInfo> _pushConstantDefine{};
      };
}


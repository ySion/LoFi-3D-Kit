//
// Created by Arzuo on 2024/7/7.
//

#pragma once

#include "Defines.h"
#include "../Helper.h"

namespace LoFi::Component::Gfx {
      class Program;
      class Kernel {
      public:
            NO_COPY_MOVE_CONS(Kernel);

            ~Kernel();

            explicit Kernel(entt::entity id);

            bool Init(const Program* program, const GfxParamCreateKernel& param);

            [[nodiscard]] const std::string& GetResourceName() const { return _resourceName; }

            [[nodiscard]] VkPipeline GetPipeline() const { return _pipeline; }

            [[nodiscard]] VkPipelineLayout GetPipelineLayout() const { return _pipelineLayout; }

            [[nodiscard]] VkPipelineLayout* GetPipelineLayoutPtr() { return &_pipelineLayout; }

            [[nodiscard]] ResourceHandle Handle() const { return {GfxEnumResourceType::Kernel, _id}; }

            [[nodiscard]] bool IsComputeKernel() const { return _isComputeKernel; }

            [[nodiscard]] bool IsGraphicsKernel() const { return !_isComputeKernel; }

            [[nodiscard]] auto& GetPushConstantRange() const { return  _pushConstantRange;}

            [[nodiscard]] auto& GetPushConstantDefine() const { return _pushConstantDefine;  }

            [[nodiscard]] GfxInfoKernelLayout GetLayout() const { return GfxInfoKernelLayout {std::bit_cast<uint64_t>(_pipelineLayout), _pushConstantRange.offset, _pushConstantRange.size}; }

            void UseDefaultPushConstant(bool use) { _useDefaultPushConstant = use; }

            bool SetConstantValue(const std::string& name, const void* data);

            bool FillConstantValue(const void* data, size_t size);

            void CmdPushConstants(VkCommandBuffer cmd) const;

      private:
            bool CreateAsGraphics(const Program* program);

            bool CreateAsCompute(const Program* program);

      private:
            entt::entity _id = entt::null;

            bool _isComputeKernel = false;

            bool _useDefaultPushConstant = true;

            VkPipeline _pipeline{};

            VkPipelineLayout _pipelineLayout{};

            VkPushConstantRange _pushConstantRange{};

            entt::dense_map<std::string, PushConstantMemberInfo> _pushConstantDefine{};

            std::vector<uint8_t> _pushConstantBuffer{};

            bool UsingBindless = false;

      private:
            std::string _resourceName{};
      };
}


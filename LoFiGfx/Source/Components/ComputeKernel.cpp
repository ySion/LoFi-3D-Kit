//
// Created by Arzuo on 2024/6/30.
//

#include "ComputeKernel.h"
#include "Program.h"
#include "../Context.h"
#include "../Message.h"

using namespace LoFi::Component;
using namespace LoFi::Internal;

ComputeKernel::ComputeKernel(entt::entity id, entt::entity program) {
      auto& world = *volkGetLoadedEcsWorld();

      if(!world.valid(id)) {
            const auto err = std::format("ComputeKernel::ComputeKernel - Invalid Entity ID\n");
            MessageManager::Log(MessageType::Warning, err);
            return;
      }

      if (!world.valid(program)) {
            const auto str = std::format("ComputeKernel::ComputeKernel - Invalid Program Entity\n");
            MessageManager::Log(MessageType::Warning, str);
            throw std::runtime_error(str);
      }

      auto prog = world.try_get<Program>(program);
      if (!prog) {
            const auto str = std::format("ComputeKernel::ComputeKernel - Program Component Not Found\n");
            MessageManager::Log(MessageType::Warning, str);
            throw std::runtime_error(str);
      }

      if (!prog->IsCompiled()) {
            const auto err = std::format("ComputeKernel::ComputeKernel - Program Not Compiled\n");
            MessageManager::Log(MessageType::Warning, err);
            throw std::runtime_error(err);
      }

      if (!prog->GetShaderModules().contains(glslang_stage_t::GLSLANG_STAGE_COMPUTE)) {
            const auto err = std::format("ComputeKernel::ComputeKernel - Compute Part Not Found In Program\n");
            MessageManager::Log(MessageType::Warning, err);
            throw std::runtime_error(err);
      }

      _pushConstantRange = prog->_pushConstantRange;

      VkPipelineLayoutCreateInfo pipeline_layout_ci{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .setLayoutCount = 1,
            .pSetLayouts = &LoFi::Context::Get()->_bindlessDescriptorSetLayout,
            .pushConstantRangeCount = 1,
            .pPushConstantRanges = &_pushConstantRange
      };

      if (vkCreatePipelineLayout(volkGetLoadedDevice(), &pipeline_layout_ci, nullptr, &_pipelineLayout) != VK_SUCCESS) {
            const auto err = std::format("ComputeKernel::ComputeKernel - Create Pipeline Layout Failed\n");
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      VkPipelineShaderStageCreateInfo cs_ci = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_COMPUTE_BIT,
            .module = prog->GetShaderModules().at(glslang_stage_t::GLSLANG_STAGE_COMPUTE).second,
            .pName = "main",
            .pSpecializationInfo = nullptr
      };

      VkComputePipelineCreateInfo pipelineInfo{
            .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
            .stage = cs_ci,
            .layout = _pipelineLayout,
            .basePipelineHandle = nullptr,
            .basePipelineIndex = 0
      };

      if(auto res = vkCreateComputePipelines(volkGetLoadedDevice(), nullptr, 1, &pipelineInfo, nullptr, &_pipeline); res != VK_SUCCESS) {
            const auto err = std::format("ComputeKernel::ComputeKernel - vkCreateComputePipelines failed with error code {}\n",  GetVkResultString(res));
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      _paramTable = prog->_paramTable;
      _paramMemberTable = prog->_paramMemberTable;
      _sampledTextureTable = prog->_sampledTextureTable;

      _bufferTable = prog->_bufferTable;
      _textureTable = prog->_textureTable;

      _marcoParserIdentifier = prog->_marcoParserIdentifier;
}

ComputeKernel::~ComputeKernel() {
      if (_pipeline) {
            const ContextResourceRecoveryInfo info {
                  .Type = ContextResourceType::PIPELINE,
                  .Resource1 = (size_t)_pipeline
            };
            Context::Get()->RecoveryContextResource(info);
      }

      if (_pipelineLayout) {
            const ContextResourceRecoveryInfo info {
                  .Type = ContextResourceType::PIPELINE_LAYOUT,
                  .Resource1 = (size_t)_pipelineLayout
            };
            Context::Get()->RecoveryContextResource(info);
      }
}

//
// Created by starr on 2024/6/23.
//

#include "GraphicKernel.h"
#include "Program.h"

#include "../Message.h"
#include "../Context.h"

using namespace LoFi::Component;
using namespace LoFi::Internal;

GraphicKernel::GraphicKernel(entt::entity id, entt::entity program) : _id(id) {

      auto& world = *volkGetLoadedEcsWorld();

      if(!world.valid(id)) {
            const auto err = std::format("GraphicKernel::GraphicKernel - Invalid Entity ID\n");
            MessageManager::Log(MessageType::Warning, err);
            throw std::runtime_error(err);
      }

      if (!world.valid(program)) {
            const auto err = std::format("GraphicKernel::CreateFromProgram - Invalid Program Entity\n");
            MessageManager::Log(MessageType::Warning, err);
            throw std::runtime_error(err);
      }

      auto prog = world.try_get<Program>(program);
      if (!prog) {
            const auto err = std::format("GraphicKernel::CreateFromProgram - Program Component Not Found\n");
            MessageManager::Log(MessageType::Warning, err);
            throw std::runtime_error(err);
      }

      if (!prog->IsCompiled()) {
            const auto err = std::format("GraphicKernel::CreateFromProgram - Program Not Compiled\n");
            MessageManager::Log(MessageType::Warning, err);
            throw std::runtime_error(err);
      }

      if (!prog->GetShaderModules().contains(glslang_stage_t::GLSLANG_STAGE_VERTEX)) {
            const auto err = std::format("GraphicKernel::CreateFromProgram - Vertex Part Not Found In Program\n");
            MessageManager::Log(MessageType::Warning, err);
            throw std::runtime_error(err);
      }

      if (!prog->GetShaderModules().contains(glslang_stage_t::GLSLANG_STAGE_FRAGMENT)) {
            const auto err = std::format("GraphicKernel::CreateFromProgram - Pixel Part Not Found In Program\n");
            MessageManager::Log(MessageType::Warning, err);
            throw std::runtime_error(err);
      }

      std::vector<VkPipelineShaderStageCreateInfo> stages{
            {
                  .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                  .stage = VK_SHADER_STAGE_VERTEX_BIT,
                  .module = prog->GetShaderModules().at(glslang_stage_t::GLSLANG_STAGE_VERTEX).second,
                  .pName = "main"
            },
            {
                  .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                  .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                  .module = prog->GetShaderModules().at(glslang_stage_t::GLSLANG_STAGE_FRAGMENT).second,
                  .pName = "main"
            }
      };

      VkPipelineViewportStateCreateInfo viewport_ci{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .scissorCount = 1,
      };

      VkPipelineMultisampleStateCreateInfo multisample_ci{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
            .sampleShadingEnable = VK_FALSE,
            .minSampleShading = 1.0f,
            .pSampleMask = nullptr,
            .alphaToCoverageEnable = VK_FALSE,
            .alphaToOneEnable = VK_FALSE
      };

      std::vector<VkDynamicState> dynamic_states{
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
      };

      VkPipelineDynamicStateCreateInfo dynamic_state_ci{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .dynamicStateCount = (uint32_t)dynamic_states.size(),
            .pDynamicStates = dynamic_states.data()
      };

      VkPipelineLayoutCreateInfo pipeline_layout_ci{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .setLayoutCount = 1,
            .pSetLayouts = &LoFi::Context::Get()->_bindlessDescriptorSetLayout,
            .pushConstantRangeCount = (uint32_t)(prog->_pushConstantRange.size == 0 ? 0 : 1),
            .pPushConstantRanges = (prog->_pushConstantRange.size == 0 ? nullptr : &prog->_pushConstantRange)
      };

      if (vkCreatePipelineLayout(volkGetLoadedDevice(), &pipeline_layout_ci, nullptr, &_pipelineLayout) != VK_SUCCESS) {
            const auto err = std::format("GraphicKernel::CreateFromProgram - Create Pipeline Layout Failed\n");
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      VkGraphicsPipelineCreateInfo pipeline_ci{
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext = &prog->_renderingCreateInfo,
            .flags = 0,
            .stageCount = (uint32_t)stages.size(),
            .pStages = stages.data(),
            .pVertexInputState = &prog->_vertexInputStateCreateInfo,
            .pInputAssemblyState = &prog->_inputAssemblyStateCreateInfo,
            .pTessellationState = nullptr,
            .pViewportState = &viewport_ci,
            .pRasterizationState = &prog->_rasterizationStateCreateInfo,
            .pMultisampleState = &multisample_ci,
            .pDepthStencilState = &prog->_depthStencilStateCreateInfo,
            .pColorBlendState = &prog->_colorBlendStateCreateInfo,
            .pDynamicState = &dynamic_state_ci,
            .layout = _pipelineLayout,
            .renderPass = nullptr,
            .subpass = 0,
            .basePipelineHandle = nullptr,
            .basePipelineIndex = 0
      };

      if (vkCreateGraphicsPipelines(volkGetLoadedDevice(), nullptr, 1, &pipeline_ci, nullptr, &_pipeline) != VK_SUCCESS) {
            const auto err = std::format("GraphicKernel::CreateFromProgram - vkCreateGraphicsPipelines Failed\n");
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      _structTable = prog->_structTable;
      _structMemberTable = prog->_structMemberTable;
      _sampledTextureTable = prog->_sampledTextureTable;
      _pushConstantRange = prog->_pushConstantRange;
      _marcoParserIdentifier = prog->_marcoParserIdentifier;
      //
      // for(int i = 0; i <  _marcoParserIdentifier.size(); i++) {
      //       if(_marcoParserIdentifier[i].second == "TEXTURE") {
      //             _sampledTextureTable[_marcoParserIdentifier[i].first] = i;
      //       }
      // }
}

GraphicKernel::~GraphicKernel() {
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

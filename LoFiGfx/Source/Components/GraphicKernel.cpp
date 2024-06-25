//
// Created by starr on 2024/6/23.
//

#include "GraphicKernel.h"
#include "Program.h"

#include "../Message.h"
#include "../Context.h"

LoFi::Component::GraphicKernel::GraphicKernel(entt::entity id) : _id(id) {

}

LoFi::Component::GraphicKernel::~GraphicKernel() {
      if(_pipeline) {
            vkDestroyPipeline(volkGetLoadedDevice(), _pipeline, nullptr);
      }

      if(_pipelineLayout){
            vkDestroyPipelineLayout(volkGetLoadedDevice(), _pipelineLayout, nullptr);
      }
}

bool LoFi::Component::GraphicKernel::CreateFromProgram(entt::entity program) {
      auto& world = *volkGetLoadedEcsWorld();

      if(!world.valid(program)) {
            auto str = std::format("GraphicKernel::CreateFromProgram - Invalid Program Entity\n");
            MessageManager::Log(MessageType::Warning, str);
            return false;
      }

      auto prog = world.try_get<LoFi::Component::Program>(program);
      if(!prog) {
            auto str = std::format("GraphicKernel::CreateFromProgram - Program Component Not Found\n");
            MessageManager::Log(MessageType::Warning, str);
            return false;
      }

      if(!prog->_vs) {
            auto str = std::format("GraphicKernel::CreateFromProgram - Vertex Shader Not Found\n");
            MessageManager::Log(MessageType::Warning, str);
            return false;
      }

      if(!prog->_ps) {
            auto str = std::format("GraphicKernel::CreateFromProgram - Pixel Shader Not Found\n");
            MessageManager::Log(MessageType::Warning, str);
            return false;
      }

      std::vector<VkPipelineShaderStageCreateInfo> stages {
            {
                  .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                  .stage = VK_SHADER_STAGE_VERTEX_BIT,
                  .module = prog->_vs,
                  .pName = "VSMain"
            },
            {
                  .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                  .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                  .module = prog->_ps,
                  .pName = "PSMain"
            }
      };

      VkViewport default_viewport = {
            .width = 1.0f,
            .height = 1.0f,
            .minDepth = 0.0f,
            .maxDepth = 1.0f
      };
      VkRect2D default_scissor = {
            .offset = {0, 0},
            .extent = {1, 1}
      };

      VkPipelineViewportStateCreateInfo viewport_ci {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .pViewports = &default_viewport,
            .scissorCount = 1,
            .pScissors = &default_scissor
      };

      VkPipelineMultisampleStateCreateInfo multisample_ci {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
            .sampleShadingEnable = VK_FALSE,
            .minSampleShading = 1.0f,
            .pSampleMask = nullptr,
            .alphaToCoverageEnable = VK_FALSE,
            .alphaToOneEnable = VK_FALSE
      };

      std::vector<VkDynamicState> dynamic_states {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
      };

      VkPipelineDynamicStateCreateInfo dynamic_state_ci {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .dynamicStateCount = (uint32_t)dynamic_states.size(),
            .pDynamicStates = dynamic_states.data()
      };

      VkPipelineLayoutCreateInfo pipeline_layout_ci {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .setLayoutCount = 1,
            .pSetLayouts = &LoFi::Context::Get()->_bindlessDescriptorSetLayout,
            .pushConstantRangeCount = (uint32_t)(prog->_pushConstantRange_prepared.size == 0 ? 0 : 1),
            .pPushConstantRanges = (prog->_pushConstantRange_prepared.size == 0 ? nullptr : &prog->_pushConstantRange_prepared)
      };

      if(vkCreatePipelineLayout(volkGetLoadedDevice(), &pipeline_layout_ci, nullptr, &_pipelineLayout) != VK_SUCCESS) {
            auto str = std::format("GraphicKernel::CreateFromProgram - vkCreatePipelineLayout Failed\n");
            MessageManager::Log(MessageType::Error, str);
            return false;
      }
      
      VkGraphicsPipelineCreateInfo pipeline_ci{
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext = &prog->_renderingCreateInfo_prepared,
            .flags = 0,
            .stageCount = (uint32_t)stages.size(),
            .pStages = stages.data(),
            .pVertexInputState = &prog->_vertexInputStateCreateInfo_prepared,
            .pInputAssemblyState = &prog->_inputAssemblyStateCreateInfo_prepared,
            .pTessellationState = nullptr,
            .pViewportState = &viewport_ci,
            .pRasterizationState = &prog->_rasterizationStateCreateInfo_prepared,
            .pMultisampleState = &multisample_ci,
            .pDepthStencilState = &prog->_depthStencilStateCreateInfo_prepared,
            .pColorBlendState = &prog->_colorBlendStateCreateInfo_prepared,
            .pDynamicState = &dynamic_state_ci,
            .layout = _pipelineLayout,
            .renderPass = nullptr,
            .subpass = 0,
            .basePipelineHandle = nullptr,
            .basePipelineIndex = 0
      };

      if(vkCreateGraphicsPipelines(volkGetLoadedDevice(), nullptr, 1, &pipeline_ci, nullptr, &_pipeline) != VK_SUCCESS) {
            auto str = std::format("GraphicKernel::CreateFromProgram - vkCreateGraphicsPipelines Failed\n");
            MessageManager::Log(MessageType::Error, str);
            return false;
      }

      return true;
}

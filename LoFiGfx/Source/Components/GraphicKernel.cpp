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
      if (_pipeline) {
            vkDestroyPipeline(volkGetLoadedDevice(), _pipeline, nullptr);
      }

      if (_pipelineLayout) {
            vkDestroyPipelineLayout(volkGetLoadedDevice(), _pipelineLayout, nullptr);
      }
}

bool LoFi::Component::GraphicKernel::CreateFromProgram(entt::entity program) {
      auto& world = *volkGetLoadedEcsWorld();

      if (!world.valid(program)) {
            auto str = std::format("GraphicKernel::CreateFromProgram - Invalid Program Entity\n");
            MessageManager::Log(MessageType::Warning, str);
            return false;
      }

      auto prog = world.try_get<LoFi::Component::Program>(program);
      if (!prog) {
            auto str = std::format("GraphicKernel::CreateFromProgram - Program Component Not Found\n");
            MessageManager::Log(MessageType::Warning, str);
            return false;
      }

      if (!prog->IsCompiled()) {
            const auto err = std::format("GraphicKernel::CreateFromProgram - Program Not Compiled\n");
            MessageManager::Log(MessageType::Warning, err);
            return false;
      }

      if (!prog->GetShaderModules().contains(glslang_stage_t::GLSLANG_STAGE_VERTEX)) {
            auto str = std::format("GraphicKernel::CreateFromProgram - Vertex Shader Not Found\n");
            MessageManager::Log(MessageType::Warning, str);
            return false;
      }

      if (!prog->GetShaderModules().contains(glslang_stage_t::GLSLANG_STAGE_FRAGMENT)) {
            auto str = std::format("GraphicKernel::CreateFromProgram - Pixel Shader Not Found\n");
            MessageManager::Log(MessageType::Warning, str);
            return false;
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

      // VkViewport default_viewport = {
      //       .width = 1.f,
      //       .height = 1.0f,
      //       .minDepth = 0.0f,
      //       .maxDepth = 1.0f
      // };
      // VkRect2D default_scissor = {
      //       .offset = {0, 0},
      //       .extent = {1, 1}
      // };

      VkPipelineViewportStateCreateInfo viewport_ci{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            //.pViewports = &default_viewport,
            .scissorCount = 1,
            // .pScissors = &default_scissor
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
            auto str = std::format("GraphicKernel::CreateFromProgram - vkCreatePipelineLayout Failed\n");
            MessageManager::Log(MessageType::Error, str);
            return false;
      }

      //empty ter:

      // std::vector<VkFormat> ff = {VK_FORMAT_R8G8B8A8_UNORM};
      //
      // VkPipelineRenderingCreateInfoKHR t1 {
      //       .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
      //       .pNext = nullptr,
      //       .viewMask = 0,
      //       .colorAttachmentCount = 1,
      //       .pColorAttachmentFormats = ff.data(),
      // };
      //
      // VkPipelineInputAssemblyStateCreateInfo vk_pipeline_input_assembly_state_create_info{};
      // vk_pipeline_input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
      // vk_pipeline_input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
      //
      // VkPipelineRasterizationStateCreateInfo vk_pipeline_rasterization_state_create_info{};
      // vk_pipeline_rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
      // vk_pipeline_rasterization_state_create_info.cullMode = VK_CULL_MODE_NONE;
      // vk_pipeline_rasterization_state_create_info.lineWidth = 1.0f;
      // vk_pipeline_rasterization_state_create_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
      //
      // VkPipelineColorBlendAttachmentState vk_pipeline_color_blend_attachment_state{};
      // vk_pipeline_color_blend_attachment_state.blendEnable = false;
      // vk_pipeline_color_blend_attachment_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
      // vk_pipeline_color_blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
      // vk_pipeline_color_blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
      // vk_pipeline_color_blend_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
      // vk_pipeline_color_blend_attachment_state.alphaBlendOp = VK_BLEND_OP_ADD;
      //
      // VkPipelineColorBlendStateCreateInfo vk_pipeline_color_blend_state_create_info{};
      // vk_pipeline_color_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
      // vk_pipeline_color_blend_state_create_info.attachmentCount = 1;
      // vk_pipeline_color_blend_state_create_info.pAttachments = &vk_pipeline_color_blend_attachment_state;
      //
      // VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
      // vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
      // vertexInputInfo.vertexBindingDescriptionCount = 0;
      // vertexInputInfo.vertexAttributeDescriptionCount = 0;
      //
      // VkPipelineMultisampleStateCreateInfo multisampling{};
      // multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
      // multisampling.sampleShadingEnable = VK_FALSE;
      // multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

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
            auto str = std::format("GraphicKernel::CreateFromProgram - vkCreateGraphicsPipelines Failed\n");
            MessageManager::Log(MessageType::Error, str);
            return false;
      }

      return true;
}

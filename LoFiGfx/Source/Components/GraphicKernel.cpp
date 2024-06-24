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

bool LoFi::Component::GraphicKernel::CreateFromProgram(entt::entity program, const GraphicKernelConfig& config) {
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

      std::vector<VkFormat> vaild_rt_format {};
      for(int i = 0; i < 8; i++) {
            if(config.RenderTargetFormat[i] != VK_FORMAT_UNDEFINED) {
                  vaild_rt_format.push_back(config.RenderTargetFormat[i]);
            }
      }

      VkPipelineRenderingCreateInfoKHR pipeline_create{VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR};
      pipeline_create.pNext                   = VK_NULL_HANDLE;
      pipeline_create.colorAttachmentCount    = vaild_rt_format.size();
      pipeline_create.pColorAttachmentFormats = vaild_rt_format.data();
      pipeline_create.depthAttachmentFormat   = config.DepthFormat;
      pipeline_create.stencilAttachmentFormat = config.StencilFormat;


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

      std::vector<VkVertexInputAttributeDescription> vertex_attribute_descs{};
      std::vector<VkVertexInputBindingDescription> vertex_binding_descs{};

      for(int i = 0; i < 8; i++) {
            if(config.VertexInputAttributeDescription[i].format != VK_FORMAT_UNDEFINED) {
                  vertex_attribute_descs.push_back(config.VertexInputAttributeDescription[i]);
            }

            if(config.VertexInputBindDescription[i].stride != 0) {
                  vertex_binding_descs.push_back(config.VertexInputBindDescription[i]);
            }
      }

      VkPipelineVertexInputStateCreateInfo vertex_input_ci{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount = (uint32_t)vertex_binding_descs.size(),
            .pVertexBindingDescriptions = vertex_binding_descs.data(),
            .vertexAttributeDescriptionCount = (uint32_t)vertex_attribute_descs.size(),
            .pVertexAttributeDescriptions = vertex_attribute_descs.data()
      };

      VkPipelineInputAssemblyStateCreateInfo input_assembly_ci = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = config.PrimitiveTopology,
            .primitiveRestartEnable = VK_FALSE
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

      VkPipelineRasterizationStateCreateInfo rasterization_ci {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .depthClampEnable = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode = config.PolygonMode,
            .cullMode = config.CullMode,
            .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
            .depthBiasEnable = VK_FALSE,
            .depthBiasConstantFactor = 0.0f,
            .depthBiasClamp = 0.0f,
            .depthBiasSlopeFactor = 0.0f,
            .lineWidth = 1.0f
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

      VkPipelineDepthStencilStateCreateInfo depth_stencil_ci {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .depthTestEnable = config.DepthTestEnable,
            .depthWriteEnable = config.DepthWriteEnable,
            .depthCompareOp = config.DepthCompareOp,
            .depthBoundsTestEnable = VK_FALSE,
            .stencilTestEnable = config.DepthTestEnable,
            .front = config.StencilOpFront,
            .back = config.StencilOpBack,
            .minDepthBounds = 0.0f,
            .maxDepthBounds = 1.0f
      };

      VkPipelineColorBlendStateCreateInfo blend = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .logicOpEnable = VK_FALSE,
            .logicOp = VK_LOGIC_OP_COPY,
            .attachmentCount = 1,
            .pAttachments = nullptr,
            .blendConstants = {0.0f, 0.0f, 0.0f, 0.0f}
      };

      VkPipelineColorBlendAttachmentState color_blend_state{};
      color_blend_state.blendEnable = false;
      color_blend_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
      color_blend_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
      color_blend_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
      color_blend_state.colorBlendOp = VK_BLEND_OP_ADD;
      color_blend_state.alphaBlendOp = VK_BLEND_OP_ADD;

      VkPipelineColorBlendStateCreateInfo color_blend_state_ci{};
      color_blend_state_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
      color_blend_state_ci.attachmentCount = 1;
      color_blend_state_ci.pAttachments = &color_blend_state;

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

      //
      // VkPushConstantRange push_constant_range {
      //       .stageFlags = VK_SHADER_STAGE_ALL,
      //       .offset = 0,
      //       .size = 2 * sizeof(float) * 4
      // };

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

#include "Kernel.h"
#include "Program.h"
#include "../GfxContext.h"
#include "../Message.h"

using namespace LoFi::Component::Gfx;
using namespace LoFi::Internal;

Kernel::~Kernel() {
      if (_pipeline) {
            const ContextResourceRecoveryInfo info{
                  .Type = ContextResourceType::PIPELINE,
                  .Resource1 = (size_t)_pipeline,
                  .ResourceName = _resourceName

            };
            GfxContext::Get()->RecoveryContextResource(info);
      }

      if (_pipelineLayout) {
            const ContextResourceRecoveryInfo info{
                  .Type = ContextResourceType::PIPELINE_LAYOUT,
                  .Resource1 = (size_t)_pipelineLayout,
                  .ResourceName = _resourceName
            };
            GfxContext::Get()->RecoveryContextResource(info);
      }
}

bool Kernel::Init(const Program* program, const GfxParamCreateKernel& param) {
      _resourceName = param.pResourceName ? param.pResourceName : std::string{};
      if (!program->IsCompiled()) {
            std::string err = "[KernelCreate] Program is not compiled.";
            if(!_resourceName.empty()) err += std::format(" - Name: \"{}\"", _resourceName);
            MessageManager::Log(MessageType::Error, err);
            return false;
      }

      if (program->IsGraphicsShader()) {
            if(CreateAsGraphics(program)) {
                  std::string str = std::format("[KernelCreate] Graphics Kernel Created.", _resourceName);
                  if(!_resourceName.empty()) str += std::format(" - Name: \"{}\"", _resourceName);
                  MessageManager::Log(MessageType::Normal, str);
                  return true;
            }
            return false;
      } else if (program->IsComputeShader()) {
            if(CreateAsCompute(program)) {
                  std::string str = std::format("[KernelCreate] Compute Kernel Created.", _resourceName);
                  if(!_resourceName.empty()) str += std::format(" - Name: \"{}\"", _resourceName);
                  MessageManager::Log(MessageType::Normal, str);
                  return true;
            } else {
                  return false;
            }
      } else {
            std::string err = "[KernelCreate] Program is not a graphics or compute shader.";
            if(!_resourceName.empty()) err += std::format(" - Name: \"{}\"", _resourceName);
            MessageManager::Log(MessageType::Error, err);
            return false;
      }
}

Kernel::Kernel(entt::entity id) : _id(id) {}

bool Kernel::CreateAsGraphics(const Program* program) {
      _pushConstantRange = program->GetPushConstantRange();
      _pushConstantDefine = program->GetPushConstantDefine();

      VkPipelineLayoutCreateInfo pipeline_layout_ci{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 1,
            .pSetLayouts = &LoFi::GfxContext::Get()->_bindlessDescriptorSetLayout,
            .pushConstantRangeCount = (uint32_t)(_pushConstantRange.size == 0 ? 0 : 1),
            .pPushConstantRanges = (_pushConstantRange.size == 0 ? nullptr : &_pushConstantRange)
      };

      if (const auto result = vkCreatePipelineLayout(volkGetLoadedDevice(), &pipeline_layout_ci, nullptr, &_pipelineLayout); result != VK_SUCCESS) {
            auto err = std::format("[Kernel::CreateAsGraphics] vkCreatePipelineLayout Failed, return {}.", ToStringVkResult(result));
            if(!_resourceName.empty()) err += std::format(" - Name: \"{}\"", _resourceName);
            MessageManager::Log(MessageType::Error, err);
            return false;
      }

      std::vector<VkPipelineShaderStageCreateInfo> stages{
            {
                  .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                  .stage = VK_SHADER_STAGE_VERTEX_BIT,
                  .module = program->GetShaderModules().at(glslang_stage_t::GLSLANG_STAGE_VERTEX).second,
                  .pName = "main"
            },
            {
                  .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                  .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                  .module = program->GetShaderModules().at(glslang_stage_t::GLSLANG_STAGE_FRAGMENT).second,
                  .pName = "main"
            }
      };

      VkPipelineViewportStateCreateInfo viewport_ci{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .scissorCount = 1,
      };

      VkPipelineMultisampleStateCreateInfo multisample_ci = program->_multiSampleCreateInfo;

      std::array dynamic_states{
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
      };

      VkPipelineDynamicStateCreateInfo dynamic_state_ci{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .dynamicStateCount = (uint32_t)dynamic_states.size(),
            .pDynamicStates = dynamic_states.data()
      };

      VkGraphicsPipelineCreateInfo pipeline_ci{
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext = &program->_renderingCreateInfo,
            .flags = 0,
            .stageCount = (uint32_t)stages.size(),
            .pStages = stages.data(),
            .pVertexInputState = &program->_vertexInputStateCreateInfo,
            .pInputAssemblyState = &program->_inputAssemblyStateCreateInfo,
            .pTessellationState = nullptr,
            .pViewportState = &viewport_ci,
            .pRasterizationState = &program->_rasterizationStateCreateInfo,
            .pMultisampleState = &multisample_ci,
            .pDepthStencilState = &program->_depthStencilStateCreateInfo,
            .pColorBlendState = &program->_colorBlendStateCreateInfo,
            .pDynamicState = &dynamic_state_ci,
            .layout = _pipelineLayout
      };

      if (const auto result = vkCreateGraphicsPipelines(volkGetLoadedDevice(), nullptr, 1, &pipeline_ci, nullptr, &_pipeline); result != VK_SUCCESS) {
            auto err = std::format("[Kernel::CreateAsGraphics] vkCreateGraphicsPipelines Failed, return {}.", ToStringVkResult(result));
            if(!_resourceName.empty()) err += std::format(" - Name: \"{}\"", _resourceName);
            MessageManager::Log(MessageType::Error, err);
            return false;
      }

      _isComputeKernel = false;
      _pushConstantBuffer.resize(_pushConstantRange.size);

      return true;
}

bool Kernel::CreateAsCompute(const Program* program) {
      _pushConstantRange = program->GetPushConstantRange();
      _pushConstantDefine = program->GetPushConstantDefine();

      VkPipelineLayoutCreateInfo pipeline_layout_ci{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 1,
            .pSetLayouts = &LoFi::GfxContext::Get()->_bindlessDescriptorSetLayout,
            .pushConstantRangeCount = 1,
            .pPushConstantRanges = &_pushConstantRange
      };

      if (const auto result = vkCreatePipelineLayout(volkGetLoadedDevice(), &pipeline_layout_ci, nullptr, &_pipelineLayout); result != VK_SUCCESS) {
            auto err = std::format("[Kernel::CreateAsCompute] vkCreatePipelineLayout Failed, return {}.", ToStringVkResult(result));
            if(!_resourceName.empty()) err += std::format(" - Name: \"{}\"", _resourceName);
            MessageManager::Log(MessageType::Error, err);
            return false;
      }

      VkPipelineShaderStageCreateInfo cs_ci = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_COMPUTE_BIT,
            .module = program->GetShaderModules().at(glslang_stage_t::GLSLANG_STAGE_COMPUTE).second,
            .pName = "main"
      };

      VkComputePipelineCreateInfo pipelineInfo{
            .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
            .stage = cs_ci,
            .layout = _pipelineLayout
      };

      if (const auto result = vkCreateComputePipelines(volkGetLoadedDevice(), nullptr, 1, &pipelineInfo, nullptr, &_pipeline); result != VK_SUCCESS) {
            auto err = std::format("[Kernel::CreateAsCompute] vkCreateComputePipelines Failed, return {}.", ToStringVkResult(result));
            if(!_resourceName.empty()) err += std::format(" - Name: \"{}\"", _resourceName);
            MessageManager::Log(MessageType::Error, err);
            return false;
      }

      _isComputeKernel = true;
      _pushConstantBuffer.resize(_pushConstantRange.size);

      return true;
}

bool Kernel::SetConstantValue(const std::string& name, const void* data) {
      if(const auto parameter_find = _pushConstantDefine.find(name); parameter_find != _pushConstantDefine.end()) {
            const auto& [Offset, Size] = parameter_find->second;
            std::memcpy(&_pushConstantBuffer.at(Offset), data, Size);
            return true;
      }
      return false;
}

bool Kernel::FillConstantValue(const void* data, size_t size) {
      if(_pushConstantBuffer.empty()) return false;
      size_t copy_size = std::min(size, _pushConstantBuffer.size());
      std::memcpy(_pushConstantBuffer.data(), data, copy_size);
      return true;
}

void Kernel::CmdPushConstants(VkCommandBuffer cmd) const {
      if(_pushConstantRange.size != 0 && _useDefaultPushConstant) {
            vkCmdPushConstants(cmd, _pipelineLayout, VK_SHADER_STAGE_ALL, 0, _pushConstantBuffer.size(), _pushConstantBuffer.data());
      }
}

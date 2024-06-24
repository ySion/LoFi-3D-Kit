#pragma once

#include "../Helper.h"
#include <any>

namespace LoFi::Component {

      class GraphicKernel;

      struct ShaderVariableRecord {
            std::string Name;
            uint32_t Offset;
            uint32_t Size;
      };

      class Program {
      public:

            NO_COPY_MOVE_CONS(Program);

            ~Program();

            explicit Program(entt::entity id);

            bool CompileFromSourceCode(std::string_view name, std::string_view source);

      private:

            bool CompileFromCode(const wchar_t* entry_point, const wchar_t* shader_type, std::vector<uint32_t>& spv);

            bool ParseSetters(std::string_view codes, entt::dense_map<std::string, std::vector<std::string>> &_setters, std::string& output_codes, std::string& error_message);

            bool VaildateSetter(std::string_view key, std::string_view value);

            bool ParseVS();

            bool ParsePS();

            friend class GraphicKernel;
      private:

            bool AnalyzeSetter(const std::pair<std::string, std::vector<std::string>>& setter, std::string& error_msg);

      private:

            entt::dense_map<std::string, std::vector<std::string>> _setters{};

            std::vector<std::string> _sampleTexture{};

            entt::entity _id {};

            std::string _programName {};

            std::string _codes {};

            VkShaderModule _vs {};

            VkShaderModule _ps {};

            VkShaderModule _cs {};

            VkShaderModule _ms {};

            VkShaderModule _as {};

            std::vector<uint32_t> _spvVS {};

            std::vector<uint32_t> _spvPS {};

            std::vector<uint32_t> _spvCS {};

            std::vector<uint32_t> _spvMS {};

            std::vector<uint32_t> _spvAS {};

      private:

            VkPipelineInputAssemblyStateCreateInfo _inputAssemblyStateCreateInfo_prepared { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
            VkPipelineInputAssemblyStateCreateInfo _inputAssemblyStateCreateInfo_temp { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};

            VkPipelineRasterizationStateCreateInfo _rasterizationStateCreateInfo_prepared { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
            VkPipelineRasterizationStateCreateInfo _rasterizationStateCreateInfo_temp { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };

            VkPipelineVertexInputStateCreateInfo _vertexInputStateCreateInfo_prepared { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
            VkPipelineVertexInputStateCreateInfo _vertexInputStateCreateInfo_temp { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };

            VkPipelineDepthStencilStateCreateInfo _depthStencilStateCreateInfo_prepared { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
            VkPipelineDepthStencilStateCreateInfo _depthStencilStateCreateInfo_temp { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };

            std::vector<VkVertexInputAttributeDescription> _vertexInputAttributeDescription_prepared {};
            std::vector<VkVertexInputAttributeDescription> _vertexInputAttributeDescription_temp {};

            std::vector<VkVertexInputBindingDescription> _vertexInputBindingDescription_prepared {};
            std::vector<VkVertexInputBindingDescription> _vertexInputBindingDescription_temp {};

            VkPipelineColorBlendStateCreateInfo _colorBlendStateCreateInfo_prepared { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
            VkPipelineColorBlendStateCreateInfo _colorBlendStateCreateInfo_temp { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };

            std::vector<VkPipelineColorBlendAttachmentState> _colorBlendAttachmentState_prepared {};
            std::vector<VkPipelineColorBlendAttachmentState> _colorBlendAttachmentState_temp {};

            VkPipelineRenderingCreateInfoKHR _renderingCreateInfo_prepared { VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR };
            VkPipelineRenderingCreateInfoKHR _renderingCreateInfo_temp { VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR };

            std::vector<VkFormat> _renderTargetFormat_prepared {};
            std::vector<VkFormat> _renderTargetFormat_temp {};

            VkPushConstantRange _pushConstantRange_prepared {};
            VkPushConstantRange _pushConstantRange_temp {};
      };
}
#pragma once

#include "../Helper.h"
#include <any>

#include "glslang/Include/glslang_c_interface.h"

namespace LoFi::Component {

      class GraphicKernel;

      struct ShaderVariableRecord {
            std::string Name;
            uint32_t Offset;
            uint32_t Size;
      };

      struct ProgramCompilerGroup {

            ProgramCompilerGroup();

            ~ProgramCompilerGroup();

            static ProgramCompilerGroup* TryInit() {
                  static ProgramCompilerGroup group {};
                  return &group;
            }
      };

      class Program {
            static inline std::map<std::string, glslang_stage_t> ShaderTypeMap {
                  {"VSMain", glslang_stage_t::GLSLANG_STAGE_VERTEX},
                  {"FSMain", glslang_stage_t::GLSLANG_STAGE_FRAGMENT},
                  {"TSMain", glslang_stage_t::GLSLANG_STAGE_TASK},
                  {"MSMain", glslang_stage_t::GLSLANG_STAGE_MESH},
                  {"CSMain", glslang_stage_t::GLSLANG_STAGE_COMPUTE},
            };
      public:

            NO_COPY_MOVE_CONS(Program);

            ~Program();

            explicit Program(entt::entity id);

            [[nodiscard]] bool CompileFromSourceCode(std::string_view name, const std::vector<std::string_view>& sources);

            [[nodiscard]] bool IsCompiled() const { return _isCompiled; }

            // TODO: LoadFromCache

      private:
            static bool CompileFromCode(const char* source, glslang_stage_t shader_type, std::vector<uint32_t>& spv, std::string& err_msg);

            bool ParseSetters(std::string_view codes, entt::dense_map<std::string, std::vector<std::string>> &_setters, std::string& output_codes, std::string& error_message, glslang_stage_t shader_type);

            bool VaildateSetter(std::string_view key, std::string_view value);

            bool ParseVS(const std::vector<uint32_t>& spv);

            bool ParseFS(const std::vector<uint32_t>& spv);

            friend class GraphicKernel;
      private:

            bool AnalyzeSetter(const std::pair<std::string, std::vector<std::string>>& setter, std::string& error_msg, glslang_stage_t shader_type);

            entt::dense_map<glslang_stage_t, std::pair<std::vector<uint32_t>, VkShaderModule>>& GetShaderModules() { return _shaderModules; }

      private:

            //entt::dense_map<std::string, std::vector<std::string>> _setters{};
            bool _isCompiled {};

            std::vector<std::string> _sampleTexture{};

            entt::dense_map<glslang_stage_t, std::pair<std::vector<uint32_t>, VkShaderModule>> _shaderModules {};

            entt::entity _id {};

            std::string _programName {};

      private:

            VkPipelineInputAssemblyStateCreateInfo _inputAssemblyStateCreateInfo {};

            VkPipelineRasterizationStateCreateInfo _rasterizationStateCreateInfo {};

            VkPipelineVertexInputStateCreateInfo _vertexInputStateCreateInfo {};

            VkPipelineDepthStencilStateCreateInfo _depthStencilStateCreateInfo {};

            std::vector<VkVertexInputAttributeDescription> _vertexInputAttributeDescription {};

            std::vector<VkVertexInputBindingDescription> _vertexInputBindingDescription {};

            VkPipelineColorBlendStateCreateInfo _colorBlendStateCreateInfo {};

            std::vector<VkPipelineColorBlendAttachmentState> _colorBlendAttachmentState {};

            VkPipelineRenderingCreateInfoKHR _renderingCreateInfo {};

            std::vector<VkFormat> _renderTargetFormat {};

            VkPushConstantRange _pushConstantRange {};
      };
}
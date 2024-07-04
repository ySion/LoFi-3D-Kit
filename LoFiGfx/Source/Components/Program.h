#pragma once

#include "../Helper.h"

#include "glslang/Include/glslang_c_interface.h"

namespace LoFi::Component {

      struct ProgramParamMemberInfo {
            uint32_t StructIndex;
            uint32_t Size;
            uint32_t Offset;
      };

      struct ProgramParamInfo {
            uint32_t Index;
            uint32_t Size;
      };

      struct ShaderVariableRecord {
            std::string Name;
            uint32_t Offset;
            uint32_t Size;
      };

      struct ProgramCompilerGroup {
            ProgramCompilerGroup();

            ~ProgramCompilerGroup();

            static ProgramCompilerGroup* TryInit() {
                  static ProgramCompilerGroup group{};
                  return &group;
            }
      };

      enum class ProgramType {
            UNKNOWN,
            GRAPHICS,
            COMPUTE,
            MESH,
            RAY_TRACING
      };

      enum class ProgramShaderReourceType {
            UNKNOWN,
            PARAM,
            SAMPLED,
            READ_BUFFER,
            READ_WRITE_BUFFER,
            READ_TEXTURE,
            READ_WRITE_TEXTURE
      };

      struct ProgramShaderReourceDefine {
            uint32_t Index = 0;
            ProgramShaderReourceType Type = ProgramShaderReourceType::UNKNOWN;
      };

      const char* HelperShaderStageToString(glslang_stage_t type);

      const char* HelperMapResourceTypeToString(ProgramShaderReourceType type);

      class Program {
            static inline std::map<std::string, glslang_stage_t> ShaderTypeMap{
                  {"VSMain", glslang_stage_t::GLSLANG_STAGE_VERTEX},
                  {"FSMain", glslang_stage_t::GLSLANG_STAGE_FRAGMENT},
                  {"TSMain", glslang_stage_t::GLSLANG_STAGE_TASK},
                  {"MSMain", glslang_stage_t::GLSLANG_STAGE_MESH},
                  {"CSMain", glslang_stage_t::GLSLANG_STAGE_COMPUTE},
            };

            static std::optional<glslang_stage_t> RecognitionShaderTypeFromSource(std::string_view source);

      public:
            NO_COPY_MOVE_CONS(Program);

            ~Program();

            explicit Program(entt::entity id, std::string_view name, const std::vector<std::string_view>& sources);

            // TODO: LoadFromCache

            [[nodiscard]] bool IsCompiled() const { return _isCompiled; }

            [[nodiscard]] bool IsGraphicsShader() const { return _shaderType == ProgramType::GRAPHICS; }

            [[nodiscard]] bool IsComputeShader() const { return _shaderType == ProgramType::COMPUTE; }

            [[nodiscard]] const auto& GetParamTable() const { return _paramTable; }

            [[nodiscard]] const auto& GetParamMemberTable() const { return _paramMemberTable; }

            [[nodiscard]] const auto& GetReourceDefineTable() const { return _resourceDefineTable; }

      private:
            static bool CompileFromCode(const char* source, glslang_stage_t shader_type, std::vector<uint32_t>& spv, std::string& err_msg);

            bool CompileCompute(std::string_view source);

            bool CompileGraphics(const std::vector<std::pair<std::string_view, glslang_stage_t>>& sources);

            bool ParseMarco(std::string_view input_code, std::string& output_codes, std::string& error_message, glslang_stage_t shader_type);

            bool ParseSetters(std::string_view codes, entt::dense_map<std::string, std::vector<std::string>>& _setters, std::string& output_codes, std::string& error_message,
            glslang_stage_t shader_type);

            bool VaildateSetter(std::string_view key, std::string_view value);

            bool ParseVS(const std::vector<uint32_t>& spv);

            bool ParseFS(const std::vector<uint32_t>& spv);

            bool ParseCS(const std::vector<uint32_t>& spv);

            friend class GraphicKernel;

            friend class ComputeKernel;

      private:
            bool AnalyzeSetter(const std::pair<std::string, std::vector<std::string>>& setter, std::string& error_msg, glslang_stage_t shader_type);

            entt::dense_map<glslang_stage_t, std::pair<std::vector<uint32_t>, VkShaderModule>>& GetShaderModules() { return _shaderModules; }

      private:
            entt::entity _id{};

            std::string _programName{};

      private:
            bool _isCompiled{};

            entt::dense_map<glslang_stage_t, std::pair<std::vector<uint32_t>, VkShaderModule>> _shaderModules{};

            entt::dense_map<std::string, ProgramParamInfo> _paramTable{};

            entt::dense_map<std::string, ProgramParamMemberInfo> _paramMemberTable{};

            entt::dense_map<std::string, ProgramShaderReourceDefine> _resourceDefineTable{};

            bool _autoVSInputStageBind = true;

            ProgramType _shaderType = ProgramType::UNKNOWN;

            entt::dense_map<uint32_t, VkVertexInputRate> _autoVSInputBindRateTable{};

      private:
            VkPipelineInputAssemblyStateCreateInfo _inputAssemblyStateCreateInfo{};

            VkPipelineRasterizationStateCreateInfo _rasterizationStateCreateInfo{};

            VkPipelineVertexInputStateCreateInfo _vertexInputStateCreateInfo{};

            VkPipelineDepthStencilStateCreateInfo _depthStencilStateCreateInfo{};

            std::vector<VkVertexInputAttributeDescription> _vertexInputAttributeDescription{};

            std::vector<VkVertexInputBindingDescription> _vertexInputBindingDescription{};

            VkPipelineColorBlendStateCreateInfo _colorBlendStateCreateInfo{};

            std::vector<VkPipelineColorBlendAttachmentState> _colorBlendAttachmentState{};

            VkPipelineRenderingCreateInfoKHR _renderingCreateInfo{};

            std::vector<VkFormat> _renderTargetFormat{};

            VkPushConstantRange _pushConstantRange{};
      };
}

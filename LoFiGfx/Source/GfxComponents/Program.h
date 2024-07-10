#pragma once

#include "Defines.h"

#include "../Helper.h"

#include "glslang/Include/glslang_c_interface.h"

namespace LoFi::Component::Gfx {

      struct ProgramCompilerGroup {
            ProgramCompilerGroup();

            ~ProgramCompilerGroup();

            static ProgramCompilerGroup* TryInit() {
                  static ProgramCompilerGroup group{};
                  return &group;
            }
      };

      const char* HelperShaderStageToString(glslang_stage_t type);

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

            [[nodiscard]] auto GetProgramType() const {return _shaderType;}

            [[nodiscard]] auto& GetPushConstantDefine() const { return _pushConstantDefine;  }

            [[nodiscard]] auto& GetPushConstantRange() const { return  _pushConstantRange;}

            [[nodiscard]] auto& GetResourceDefine() const { return _shaderResourceDefine;  }

            [[nodiscard]] uint32_t GetParameterTableSize() const { return _parameterTableSize;  }

            [[nodiscard]] const entt::dense_map<glslang_stage_t, std::pair<std::vector<uint32_t>, VkShaderModule>>& GetShaderModules() const { return _shaderModules; }

      private:
            static bool CompileFromCode(const char* source, glslang_stage_t shader_type, std::vector<uint32_t>& spv, std::string& err_msg);

            bool ParseMarco(std::string_view input_code, std::string& output_codes, std::string& error_message, glslang_stage_t shader_type);

            void CompileCompute(std::string_view source);

            void CompileGraphics(const std::vector<std::pair<std::string_view, glslang_stage_t>>& sources);

            bool ParseSetters(std::string_view codes, entt::dense_map<std::string, std::vector<std::string>>& _setters, std::string& output_codes, std::string& error_message,
            glslang_stage_t shader_type);

            bool AnalyzeSetter(const std::pair<std::string, std::vector<std::string>>& setter, std::string& error_msg, glslang_stage_t shader_type);

            bool VaildateSetter(std::string_view key, std::string_view value);

            void ParseVS(const std::vector<uint32_t>& spv);

            void ParseFS(const std::vector<uint32_t>& spv);

            void ParseCS(const std::vector<uint32_t>& spv);

            friend class Kernel;

      private:
            entt::entity _id{};

            std::string _programName{};

      private:
            bool _isCompiled{};

            entt::dense_map<glslang_stage_t, std::pair<std::vector<uint32_t>, VkShaderModule>> _shaderModules{};

            entt::dense_map<std::string, std::string> _shaderBufferResourceDefineCode{};

            entt::dense_map<std::string, ShaderResourceInfo> _shaderResourceDefine{};

            bool _autoVSInputStageBind = true;

            uint32_t _parameterTableSize {};

            ProgramType _shaderType = ProgramType::UNKNOWN;

            entt::dense_map<uint32_t, VkVertexInputRate> _autoVSInputBindRateTable{};

            entt::dense_map<std::string, PushConstantMemberInfo> _pushConstantDefine{};

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

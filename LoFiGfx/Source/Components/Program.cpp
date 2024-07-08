#include "Program.h"

#include "../Context.h"
#include "../Message.h"

#include "../Third/spirv-cross/spirv_cross.hpp"
#include "../Third/glslang/Public/resource_limits_c.h"

using namespace LoFi::Component;
using namespace LoFi::Internal;

const char* LoFi::Component::HelperShaderStageToString(glslang_stage_t type) {
      switch (type) {
            case GLSLANG_STAGE_VERTEX: return "Vertex_shader";
            case GLSLANG_STAGE_TESSCONTROL: return "Tesscontrol_shader";
            case GLSLANG_STAGE_TESSEVALUATION: return "Tessevaluation_shader";
            case GLSLANG_STAGE_GEOMETRY: return "Geometry_shader";
            case GLSLANG_STAGE_FRAGMENT: return "Fragment_shader";
            case GLSLANG_STAGE_COMPUTE: return "Compute_shader";
            case GLSLANG_STAGE_RAYGEN: return "Raygen_shader";
            case GLSLANG_STAGE_INTERSECT: return "Intersect_shader";
            case GLSLANG_STAGE_ANYHIT: return "Anyhit_shader";
            case GLSLANG_STAGE_CLOSESTHIT: return "Closesthit_shader";
            case GLSLANG_STAGE_MISS: return "Miss_shader";
            case GLSLANG_STAGE_CALLABLE: return "Callable_shader";
            case GLSLANG_STAGE_TASK: return "Task_shader";
            case GLSLANG_STAGE_MESH: return "Mesh_shader";
            default: return "err shader";
      }
}

std::optional<glslang_stage_t> Program::RecognitionShaderTypeFromSource(std::string_view source) {
      for (auto& i : ShaderTypeMap) {
            if (source.find(i.first) != std::string::npos) {
                  return i.second;
            }
      }
      return std::nullopt;
}


ProgramCompilerGroup::ProgramCompilerGroup() {
      glslang_initialize_process();
}

ProgramCompilerGroup::~ProgramCompilerGroup() {
      glslang_finalize_process();
}

Program::~Program() {
      for (auto& value : _shaderModules | std::views::values) {
            if (value.second != VK_NULL_HANDLE) {
                  vkDestroyShaderModule(volkGetLoadedDevice(), value.second, nullptr);
            }
      }
      _shaderModules.clear();
}

Program::Program(entt::entity id, std::string_view name, const std::vector<std::string_view>& sources) : _id(id) {
      auto& world = *volkGetLoadedEcsWorld();

      if (!world.valid(id)) {
            const auto err = "Program::Program: id is invalid";
            throw std::runtime_error(err);
      }

      ProgramCompilerGroup::TryInit();

      _programName = name;

      std::vector<std::pair<std::string_view, glslang_stage_t>> graphics_sources{};

      entt::dense_map<glslang_stage_t, std::string_view> source_types{};
      for (auto source : sources) {
            auto shader_type = RecognitionShaderTypeFromSource(source);
            if (shader_type.has_value()) {
                  if (source_types.contains(shader_type.value())) {
                        const auto err = std::format("Program::Program - Shader Type {} is repeat.", HelperShaderStageToString(shader_type.value()));
                        throw std::runtime_error(err);
                  } else {
                        source_types.insert({shader_type.value(), source});
                  }
            }
      }

      //try pick graphics sources
      if (source_types.contains(glslang_stage_t::GLSLANG_STAGE_VERTEX) && source_types.contains(glslang_stage_t::GLSLANG_STAGE_FRAGMENT)) {
            graphics_sources.emplace_back(source_types.at(glslang_stage_t::GLSLANG_STAGE_VERTEX), glslang_stage_t::GLSLANG_STAGE_VERTEX);
            graphics_sources.emplace_back(source_types.at(glslang_stage_t::GLSLANG_STAGE_FRAGMENT), glslang_stage_t::GLSLANG_STAGE_FRAGMENT);
            CompileGraphics(graphics_sources);
            _shaderType = ProgramType::GRAPHICS;
      } else if (source_types.contains(glslang_stage_t::GLSLANG_STAGE_COMPUTE)) {
            CompileCompute(source_types.at(glslang_stage_t::GLSLANG_STAGE_COMPUTE));
            _shaderType = ProgramType::COMPUTE;
      } else {
            const auto err = std::format("Program::Program - Failed to find shader type for shader program \"{}\", unknown shader type.", _programName);
            throw std::runtime_error(err);
      }

      _isCompiled = true;
}


void Program::CompileCompute(std::string_view source) {
      entt::dense_map<std::string, std::vector<std::string>> setters{}; // TODO: setter

      _pushConstantRange = VkPushConstantRange{
            .stageFlags = VK_SHADER_STAGE_ALL,
            .offset = 0,
            .size = 0
      };

      VkShaderModuleCreateInfo shader_ci{
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .codeSize = 0,
            .pCode = nullptr
      };

      std::string shader_type_str = HelperShaderStageToString(GLSLANG_STAGE_COMPUTE);

      std::string source_output{};
      std::string marco_parse_err_msg{};
      if (!ParseMarco(source, source_output, marco_parse_err_msg, GLSLANG_STAGE_COMPUTE)) {
            auto err = std::format("Program::ParseMarco - Failed to parse marco for shader program \"{}\" shader type :\"{}\".\nMarcoCompiler:\n{}",
            _programName, shader_type_str, marco_parse_err_msg);
            throw std::runtime_error(err);
      }

      std::string header{};
      header += "#extension GL_EXT_nonuniform_qualifier : enable\n";
      header += "#extension GL_EXT_buffer_reference : enable\n";
      header += "#extension GL_EXT_scalar_block_layout : enable\n";
      header += "#extension GL_EXT_buffer_reference2 : require\n";

      header += "#define BindlessSamplerBinding 0\n";
      header += "#define BindlessStorageTextureBinding 1\n";

      header += "layout(set = 0, binding = BindlessSamplerBinding) uniform sampler1D _bindlessSamper1D[];\n";
      header += "layout(set = 0, binding = BindlessSamplerBinding) uniform sampler2D _bindlessSamper2D[];\n";
      header += "layout(set = 0, binding = BindlessSamplerBinding) uniform sampler3D _bindlessSamper3D[];\n";
      header += "layout(set = 0, binding = BindlessSamplerBinding) uniform samplerCube _bindlessSamperCube[];\n";
      header += "layout(set = 0, binding = BindlessSamplerBinding) uniform sampler1DArray _bindlessSampler1DArray[];\n";
      header += "layout(set = 0, binding = BindlessSamplerBinding) uniform sampler2DArray _bindlessSampler2DArray[];\n";

      header += "#define GetTextureID(Handle) pushConstant.parameterTable.Handle\n";
      header += "#define GetSampled1D(Handle) _bindlessSamper1D[nonuniformEXT(uint(Handle))]\n";
      header += "#define GetSampled2D(Handle) _bindlessSamper2D[nonuniformEXT(uint(Handle))]\n";
      header += "#define GetSampled3D(Handle) _bindlessSamper3D[nonuniformEXT(uint(Handle))]\n";
      header += "#define GetSampledCube(Handle) _bindlessSamperCube[nonuniformEXT(uint(Handle))]\n";
      header += "#define GetSampled1DArray(Handle) _bindlessSampler1DArray[nonuniformEXT(uint(Handle))]\n";
      header += "#define GetSampled2DArray(Handle) _bindlessSampler2DArray[nonuniformEXT(uint(Handle))]\n";

      header += "#define GetBuffer(Name) pushConstant.parameterTable.ptr##Name\n";

      //constant parameter table
      if(!_shaderResourceDefine.empty()) {
            std::string buffer_pointer_define_code{};
            for(const auto& item : _shaderBufferResourceDefineCode) {
                  buffer_pointer_define_code += item.second + "\n";
            }

            std::string parameter_table_code = "layout(buffer_reference, scalar) readonly buffer ParameterTable {\n";

            uint32_t resource_offset = 0;
            bool align_to_8 = false;
            uint32_t index_order = 0;
            for(const auto& item : _shaderResourceDefine) {
                  auto& [resource_name, info] = item;
                  if(info.Type == ShaderResource::READ_BUFFER || info.Type == ShaderResource::WRITE_BUFFER || info.Type == ShaderResource::READ_WRITE_BUFFER) {
                        parameter_table_code += std::format("\t {} ptr{};\n", resource_name, resource_name);
                        info.Offset = resource_offset;
                        info.Size = 8;
                        resource_offset += 8;
                        align_to_8 = true;

                        info.Index = index_order;
                        index_order++;
                  }
            }

            for(const auto& item : _shaderResourceDefine) {
                  const auto& [resource_name, info] = item;
                  if(info.Type == ShaderResource::READ_TEXTURE || info.Type == ShaderResource::WRITE_TEXTURE || info.Type == ShaderResource::READ_WRITE_TEXTURE || info.Type == ShaderResource::SAMPLED_TEXTURE) {
                        parameter_table_code += std::format("\t uint {};\n", resource_name);
                        info.Offset = resource_offset;
                        info.Size = 4;
                        resource_offset += 4;

                        info.Index = index_order;
                        index_order++;
                  }
            }

            if(align_to_8 && resource_offset % 8 != 0) {
                  resource_offset += 8 - (resource_offset % 8);
                  _parameterTableSize = resource_offset;
            } else {
                  _parameterTableSize = resource_offset;
            }

            parameter_table_code+= "};\n";

            //gen push constant
            parameter_table_code += "layout(push_constant) uniform readonly PushConstant {\n";
            parameter_table_code += "\t ParameterTable parameterTable;\n";
            parameter_table_code += "} pushConstant;\n";

            //inssert to head
            header += buffer_pointer_define_code;
            header += parameter_table_code;
      }

      source_output.insert(source_output.begin(), header.begin(), header.end());
      printf("\n=============================\n%s\n=============================\n", source_output.c_str());

      // Parse Stage
      setters.clear();

      std::string setter_parse_err_msg{};
      std::string source_code_output{};

      std::vector<uint32_t> spv{};
      VkShaderModule shader_module{};

      if (!ParseSetters(source_output, setters, source_code_output, setter_parse_err_msg, GLSLANG_STAGE_COMPUTE)) {
            auto err = std::format("Program::CompileCompute - Failed to parse setters for shader program \"{}\" shader type :\"{}\".\nSetterCompiler:\n{}",
            _programName, shader_type_str, setter_parse_err_msg);
            throw std::runtime_error(err);
      }

      if (!CompileFromCode(source_code_output.data(), GLSLANG_STAGE_COMPUTE, spv, setter_parse_err_msg)) {
            const auto err = std::format("Program::CompileCompute - Failed to compile shader program \"{}\", shader type :\"{}\".\nShaderCompiler:\n{}",
            _programName, shader_type_str, setter_parse_err_msg);
            throw std::runtime_error(err);
      }

      ParseCS(spv);

      shader_ci.codeSize = spv.size() * sizeof(uint32_t);
      shader_ci.pCode = spv.data();

      if (auto res = vkCreateShaderModule(volkGetLoadedDevice(), &shader_ci, nullptr, &shader_module); res != VK_SUCCESS) {
            const auto err = std::format("Program::CompileCompute - Failed to create shader module for shader program \"{}\", shader type :\"{}\".",
            _programName, shader_type_str);
            throw std::runtime_error(err);
      }

      _shaderModules[GLSLANG_STAGE_COMPUTE] = std::make_pair(std::move(spv), shader_module);
      const auto success = std::format("Program::CompileCompute - Successfully compiled Compute program \"{}\".", _programName);
      MessageManager::Log(MessageType::Normal, success);
}

void Program::CompileGraphics(const std::vector<std::pair<std::string_view, glslang_stage_t>>& sources) {
      //Init Pipeline CIs, use default profile
      _inputAssemblyStateCreateInfo = VkPipelineInputAssemblyStateCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .primitiveRestartEnable = VK_FALSE
      };

      _rasterizationStateCreateInfo = VkPipelineRasterizationStateCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .depthClampEnable = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .cullMode = VK_CULL_MODE_BACK_BIT,
            .frontFace = VK_FRONT_FACE_CLOCKWISE,
            .depthBiasEnable = VK_FALSE,
            .depthBiasConstantFactor = 0.0f,
            .depthBiasClamp = 0.0f,
            .depthBiasSlopeFactor = 0.0f,
            .lineWidth = 1.0f
      };

      _vertexInputStateCreateInfo = VkPipelineVertexInputStateCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .vertexBindingDescriptionCount = 0,
            .pVertexBindingDescriptions = nullptr,
            .vertexAttributeDescriptionCount = 0,
            .pVertexAttributeDescriptions = nullptr
      };

      _depthStencilStateCreateInfo = VkPipelineDepthStencilStateCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .depthTestEnable = VK_FALSE,
            .depthWriteEnable = VK_FALSE,
            .depthCompareOp = VK_COMPARE_OP_LESS,
            .depthBoundsTestEnable = VK_FALSE,
            .stencilTestEnable = VK_FALSE,
            .front = {},
            .back = {},
            .minDepthBounds = 0.0f,
            .maxDepthBounds = 1.0f
      };

      _colorBlendStateCreateInfo = VkPipelineColorBlendStateCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .logicOpEnable = VK_FALSE,
            .logicOp = VK_LOGIC_OP_COPY,
            .attachmentCount = 0,
            .pAttachments = nullptr,
            .blendConstants = {0.0f, 0.0f, 0.0f, 0.0f}
      };

      _renderingCreateInfo = VkPipelineRenderingCreateInfoKHR{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
            .pNext = nullptr,
            .viewMask = 0,
            .colorAttachmentCount = 0,
            .pColorAttachmentFormats = nullptr,
            .depthAttachmentFormat = VK_FORMAT_UNDEFINED,
            .stencilAttachmentFormat = VK_FORMAT_UNDEFINED
      };

      _pushConstantRange = VkPushConstantRange{
            .stageFlags = VK_SHADER_STAGE_ALL,
            .offset = 0,
            .size = 0
      };

      VkShaderModuleCreateInfo shader_ci{
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .codeSize = 0,
            .pCode = nullptr
      };

      _vertexInputAttributeDescription.clear();
      _vertexInputBindingDescription.clear();
      _colorBlendAttachmentState.clear();
      _renderTargetFormat.clear();

      entt::dense_map<std::string, std::vector<std::string>> setters{}; // TODO: setter

      std::vector<std::pair<std::string, glslang_stage_t>> source_after_marco{};

      for (auto item : sources) {
            glslang_stage_t shader_type = item.second;
            std::string_view source = item.first;

            std::string shader_type_str = HelperShaderStageToString(shader_type);

            std::string marco_parse_source_output{};
            std::string marco_parse_err_msg{};
            if (!ParseMarco(source, marco_parse_source_output, marco_parse_err_msg, shader_type)) {
                  const auto err = std::format("Program::ParseMarco - Failed to parse marco for shader program \"{}\" shader type :\"{}\".\nMarcoCompiler:\n{}",
                  _programName, shader_type_str, marco_parse_err_msg);
                  throw std::runtime_error(err);
            }
            source_after_marco.emplace_back(std::move(marco_parse_source_output), shader_type);
      }

      //Head Gen

      std::string header{};
      header += "#extension GL_EXT_nonuniform_qualifier : enable\n";
      header += "#extension GL_EXT_buffer_reference : enable\n";
      header += "#extension GL_EXT_scalar_block_layout : enable\n";
      header += "#extension GL_EXT_buffer_reference2 : require\n";

      header += "#define BindlessSamplerBinding 0\n";
      header += "#define BindlessStorageTextureBinding 1\n";

      header += "layout(set = 0, binding = BindlessSamplerBinding) uniform sampler1D _bindlessSamper1D[];\n";
      header += "layout(set = 0, binding = BindlessSamplerBinding) uniform sampler2D _bindlessSamper2D[];\n";
      header += "layout(set = 0, binding = BindlessSamplerBinding) uniform sampler3D _bindlessSamper3D[];\n";
      header += "layout(set = 0, binding = BindlessSamplerBinding) uniform samplerCube _bindlessSamperCube[];\n";
      header += "layout(set = 0, binding = BindlessSamplerBinding) uniform sampler1DArray _bindlessSampler1DArray[];\n";
      header += "layout(set = 0, binding = BindlessSamplerBinding) uniform sampler2DArray _bindlessSampler2DArray[];\n";

      header += "#define GetTextureID(TextureName) pushConstant.parameterTable.TextureName\n";

      header += "#define GetSampled1D(ID) _bindlessSamper1D[nonuniformEXT(uint(ID))]\n";
      header += "#define GetSampled2D(ID) _bindlessSamper2D[nonuniformEXT(uint(ID))]\n";
      header += "#define GetSampled3D(ID) _bindlessSamper3D[nonuniformEXT(uint(ID))]\n";
      header += "#define GetSampledCube(ID) _bindlessSamperCube[nonuniformEXT(uint(ID))]\n";
      header += "#define GetSampled1DArray(ID) _bindlessSampler1DArray[nonuniformEXT(uint(ID))]\n";
      header += "#define GetSampled2DArray(ID) _bindlessSampler2DArray[nonuniformEXT(uint(ID))]\n";

      header += "#define GetBuffer(Name) pushConstant.parameterTable.ptr##Name\n";

      //constant parameter table
      if(!_shaderResourceDefine.empty()) {
            std::string buffer_pointer_define_code{};
            for(const auto& item : _shaderBufferResourceDefineCode) {
                  buffer_pointer_define_code += item.second + "\n";
            }

            std::string parameter_table_code = "layout(buffer_reference, scalar) readonly buffer ParameterTable {\n";

            uint32_t resource_offset = 0;
            bool align_to_8 = false;
            for(const auto& item : _shaderResourceDefine) {
                  auto& [resource_name, info] = item;
                  if(info.Type == ShaderResource::READ_BUFFER || info.Type == ShaderResource::WRITE_BUFFER || info.Type == ShaderResource::READ_WRITE_BUFFER) {
                        parameter_table_code += std::format("\t {} ptr{};\n", resource_name, resource_name);
                        info.Offset = resource_offset;
                        info.Size = 8;
                        resource_offset += 8;
                        align_to_8 = true;
                  }
            }

            for(const auto& item : _shaderResourceDefine) {
                  const auto& [resource_name, info] = item;
                  if(info.Type == ShaderResource::READ_TEXTURE || info.Type == ShaderResource::WRITE_TEXTURE || info.Type == ShaderResource::READ_WRITE_TEXTURE || info.Type == ShaderResource::SAMPLED_TEXTURE) {
                        parameter_table_code += std::format("\t uint {};\n", resource_name);
                        info.Offset = resource_offset;
                        info.Size = 4;
                        resource_offset += 4;
                  }
            }

            if(align_to_8 && resource_offset % 8 != 0) {
                  resource_offset += 8 - (resource_offset % 8);
                  _parameterTableSize = resource_offset;
            } else {
                  _parameterTableSize = resource_offset;
            }

            parameter_table_code+= "};\n";

            //gen push constant
            parameter_table_code += "layout(push_constant) uniform readonly PushConstant {\n";
            parameter_table_code += "\t ParameterTable parameterTable;\n";
            parameter_table_code += "} pushConstant;\n";

            //inssert to head
            header += buffer_pointer_define_code;
            header += parameter_table_code;
      }


      for (auto& key : source_after_marco | std::views::keys) {
            std::string& source = key;
            source.insert(source.begin(), header.begin(), header.end());
            printf("\n=============================\n%s\n=============================\n", source.c_str());
      }

      for (const auto& item : source_after_marco) {
            glslang_stage_t shader_type = item.second;
            const std::string& source = item.first;

            setters.clear();

            std::string setter_parse_err_msg{};
            std::string source_code_output{};

            std::string shader_type_str = HelperShaderStageToString(shader_type);
            std::vector<uint32_t> spv{};
            VkShaderModule shader_module{};

            if (!ParseSetters(source, setters, source_code_output, setter_parse_err_msg, shader_type)) {
                  auto err = std::format("Program::CompileGraphics - Failed to parse setters for shader program \"{}\" shader type :\"{}\".\nSetterCompiler:\n{}",
                  _programName, shader_type_str, setter_parse_err_msg);
                  throw std::runtime_error(err);
            }

            if (!CompileFromCode(source_code_output.data(), shader_type, spv, setter_parse_err_msg)) {
                  const auto err = std::format("Program::CompileGraphics - Failed to compile shader program \"{}\", shader type :\"{}\".\nShaderCompiler:\n{}",
                  _programName, shader_type_str, setter_parse_err_msg);
                  throw std::runtime_error(err);
            }

            switch (shader_type) {
                  case GLSLANG_STAGE_VERTEX: ParseVS(spv);
                        break;
                  case GLSLANG_STAGE_FRAGMENT: ParseFS(spv);
                        break;
                  default:
                        {
                              const auto err = std::format("Program::CompileGraphics - Failed to parse shader program \"{}\", shader type :\"{}\".", _programName, shader_type_str);
                              throw std::runtime_error(err);
                        }
                        break;
            }

            shader_ci.codeSize = spv.size() * sizeof(uint32_t);
            shader_ci.pCode = spv.data();

            if (auto res = vkCreateShaderModule(volkGetLoadedDevice(), &shader_ci, nullptr, &shader_module); res != VK_SUCCESS) {
                  const auto err = std::format("Program::CompileGraphics - Failed to create shader module for shader program \"{}\", shader type :\"{}\".",
                  _programName, shader_type_str);
                  throw std::runtime_error(err);
            }

            _shaderModules[shader_type] = std::make_pair(std::move(spv), shader_module);
      }

      const auto success = std::format("Program::CompileGraphics - Successfully compiled Graphics program \"{}\".", _programName);
      MessageManager::Log(MessageType::Normal, success);
}

bool Program::CompileFromCode(const char* source, glslang_stage_t shader_type, std::vector<uint32_t>& spv, std::string& err_msg) {
      const glslang_input_t input = {
            .language = GLSLANG_SOURCE_GLSL,
            .stage = shader_type,
            .client = GLSLANG_CLIENT_VULKAN,
            .client_version = GLSLANG_TARGET_VULKAN_1_3,
            .target_language = GLSLANG_TARGET_SPV,
            .target_language_version = GLSLANG_TARGET_SPV_1_6,
            .code = source,
            .default_version = 460,
            .default_profile = GLSLANG_NO_PROFILE,
            .force_default_version_and_profile = false,
            .forward_compatible = false,
            .messages = GLSLANG_MSG_DEFAULT_BIT,
            .resource = glslang_default_resource(),
      };

      glslang_shader_t* shader = glslang_shader_create(&input);
      int options = 0;
      options |= GLSLANG_SHADER_AUTO_MAP_BINDINGS;
      options |= GLSLANG_SHADER_AUTO_MAP_LOCATIONS;
      options |= GLSLANG_SHADER_VULKAN_RULES_RELAXED;
      glslang_shader_set_options(shader, options);
      if (!glslang_shader_preprocess(shader, &input)) {
            err_msg = std::format("Failed to preprocess shader: {}.", glslang_shader_get_info_log(shader));
            glslang_shader_delete(shader);
            return false;
      }

      if (!glslang_shader_parse(shader, &input)) {
            err_msg = std::format("Failed to parse shader: {}.", glslang_shader_get_info_log(shader));
            glslang_shader_delete(shader);
            return false;
      }

      glslang_program_t* program = glslang_program_create();
      glslang_program_add_shader(program, shader);

      if (!glslang_program_link(program, GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT)) {
            err_msg = std::format("Failed to link shader: {}.", glslang_program_get_info_log(program));
            glslang_program_delete(program);
            glslang_shader_delete(shader);
            return false;
      }

      // glslang_spv_options_t spv_options = {
      //       .generate_debug_info = false,
      //       .strip_debug_info = false,
      //       .disable_optimizer = false,
      //       .optimize_size = true,
      //       .disassemble = false,
      //       .validate = true,
      //       .emit_nonsemantic_shader_debug_info = false,
      //       .emit_nonsemantic_shader_debug_source = false,
      //       .compile_only = false,
      // };

      //glslang_program_SPIRV_generate_with_options(program, stage, &spv_options);
      glslang_program_SPIRV_generate(program, shader_type);

      spv.resize(glslang_program_SPIRV_get_size(program));
      memcpy(spv.data(), glslang_program_SPIRV_get_ptr(program), glslang_program_SPIRV_get_size(program) * sizeof(uint32_t));
      glslang_program_delete(program);
      glslang_shader_delete(shader);

      return true;
}

bool Program::ParseMarco(std::string_view input_code, std::string& output_codes, std::string& error_message, glslang_stage_t shader_type) {
      std::string_view source_code = input_code;

      //return <word, code after word>
      auto EatWord = [](const std::string_view str) -> std::optional<std::pair<std::string_view, std::string_view>> {
            std::string_view::size_type word_start{};
            std::string_view::size_type index{};

            // eat space
            for (; word_start < str.size(); word_start++) {
                  if (str[word_start] == ' ' || str[word_start] == '\t' || str[word_start] == '\r' || str[word_start] == '\n') { continue; } else { break; }
            }

            index = word_start;
            for (; index < str.size(); index++) {
                  if (std::isalnum(str[index]) || str[index] == '_') { continue; } else { break; }
            }

            if (index == word_start) {
                  return std::nullopt;
            } else {
                  if (index >= str.size()) {
                        return std::make_pair(str.substr(word_start, index - word_start), std::string_view{});
                  } else {
                        return std::make_pair(str.substr(word_start, index - word_start), str.substr(index, str.size() - index));
                  }
            }
      };

      //return <code in front of marco, code after marco>
      auto FindMarco = [](std::string_view codes, std::string_view marco) -> std::pair<std::string_view, std::string_view> {
            if (const auto find_pos = codes.find(marco); find_pos != std::string_view::npos) {
                  return std::make_pair(codes.substr(0, find_pos), codes.substr(find_pos + marco.size(), codes.size() - find_pos - marco.size()));
            } else {
                  return std::make_pair(codes, std::string_view{});
            }
      };

      auto FindFrontMarcos = [&](std::string_view codes, const std::vector<std::string_view>& marcos) -> std::tuple<std::string_view, std::string_view, std::string_view> {
            size_t find_pos = std::string_view::npos;
            std::string_view marco = {};
            for (auto i : marcos) {
                  if (const auto res = codes.find(i); res != std::string_view::npos) {
                        if (res < find_pos) {
                              find_pos = res;
                              marco = i;
                        }
                  }
            }

            if (!marco.empty()) {
                  const auto temp = FindMarco(codes, marco);
                  return std::make_tuple(temp.first, temp.second, marco);
            } else {
                  return std::make_tuple(codes, std::string_view{}, std::string{});
            }
      };

      // return <code block, after code block>
      auto EatCodeBlock = [](std::string_view str) -> std::optional<std::pair<std::string_view, std::string_view>> {
            int64_t index = 0;

            int64_t start = 0;
            int64_t end = 0;
            int64_t bracket_match_index = 0;

            bool finded = false;
            while (index < str.size()) {
                  if (str[index] == '{') {
                        if (finded) {
                              bracket_match_index++;
                        } else {
                              finded = true;
                              start = index;
                              bracket_match_index++;
                        }
                  } else if (str[index] == '}') {
                        if (finded) {
                              bracket_match_index--;
                              if (bracket_match_index == 0) {
                                    end = index;

                                    if (end + 1 >= str.size())
                                          return std::make_pair(std::string_view{str.begin() + start, str.begin() + end + 1}, std::string_view{});
                                    else
                                          return std::make_pair(
                                          std::string_view{str.begin() + start, str.begin() + end + 1},
                                          std::string_view{str.begin() + end + 1, str.begin() + (int64_t)str.size()}
                                          );
                              }
                        }
                  }
                  index++;
            }

            return std::nullopt;
      };

      output_codes = "";
      std::vector<std::string_view> marcos_to_find{"PUSH_CONSTANT", "SAMPLED", "RWBUFFER", "RBUFFER", "WBUFFER" , "RWTEXTURE", "RTEXTURE", "WTEXTURE"};
      while (true) {
            auto [front, after, marco] = FindFrontMarcos(source_code, marcos_to_find);

            if (after.empty()) {
                  output_codes += front;
                  break;
            } else {
                  output_codes += front;
                  source_code = after;
                  if (marco == "PUSH_CONSTANT") {
                        auto struct_type_eat_word = EatWord(source_code);
                        if (!struct_type_eat_word.has_value()) {
                              error_message = std::format("Program::ParseMarco - Invalid statement, need a name after \"{}\" key word.", marco);
                              return false;
                        }

                        auto [struct_typename, code_after_struct_typename] = struct_type_eat_word.value(); // typename
                        source_code = code_after_struct_typename;

                        const auto eat_code_block = EatCodeBlock(source_code);
                        if (!eat_code_block.has_value()) {
                              error_message = std::format("Program::ParseMarco - Invalid statement, need a code block after PUSH_CONSATNT");
                              return false;
                        }

                        const auto [code_block, code_after_block] = eat_code_block.value();

                        //output_codes += std::format("layout(std140, set = 0, binding = BindlessStorageBinding) readonly buffer {} {} _bindless{}[];", struct_typename, code_block, struct_typename);

                        output_codes += std::format("layout(push_constant) uniform readonly Type{} {} {};", struct_typename, code_block, struct_typename);
                        source_code = code_after_block;

                  } else if (marco == "RBUFFER") {
                        auto struct_type_eat_word = EatWord(source_code);
                        if (!struct_type_eat_word.has_value()) {
                              error_message = std::format("Program::ParseMarco - Invalid statement, need a RBUFFER name after \"{}\" key word.", marco);
                              return false;
                        }

                        auto [struct_typename, code_after_struct_typename] = struct_type_eat_word.value(); // typename
                        source_code = code_after_struct_typename;

                        const auto eat_code_block = EatCodeBlock(source_code);
                        if (!eat_code_block.has_value()) {
                              error_message = std::format("Program::ParseMarco - Invalid statement, need a code block after RBUFFER type name \"{}\".", struct_typename);
                              return false;
                        }

                        const auto [code_block, code_after_block] = eat_code_block.value();
                        //output_codes += std::format("layout(buffer_reference, scalar) readonly buffer {} {};", struct_typename, code_block);
                        source_code = code_after_block;

                        const auto buffer_pointer_define_code =  std::format("layout(buffer_reference, scalar) readonly buffer {} {};", struct_typename, code_block);

                        const auto struct_typename_str = std::string{struct_typename};
                        if(const auto finder = _shaderBufferResourceDefineCode.find(struct_typename_str); finder != std::end(_shaderBufferResourceDefineCode)) {
                              error_message = std::format("Program::ParseMarco - Invalid statement,Resource name \"{}\" is repeat, use it directly.", struct_typename_str);
                              return false;
                        } else {
                              _shaderBufferResourceDefineCode.emplace(struct_typename_str, buffer_pointer_define_code);
                              _shaderResourceDefine.emplace(struct_typename_str, ShaderResourceInfo {ShaderResource::READ_BUFFER});
                        }

                  } else if (marco == "RWBUFFER") {
                        auto struct_type_eat_word = EatWord(source_code);
                        if (!struct_type_eat_word.has_value()) {
                              error_message = std::format("Program::ParseMarco - Invalid statement, need a RWBUFFER name after \"{}\" key word.", marco);
                              return false;
                        }

                        auto [struct_typename, code_after_struct_typename] = struct_type_eat_word.value(); // typename
                        source_code = code_after_struct_typename;

                        const auto eat_code_block = EatCodeBlock(source_code);
                        if (!eat_code_block.has_value()) {
                              error_message = std::format("Program::ParseMarco - Invalid statement, need a code block after RWBUFFER type name \"{}\".", struct_typename);
                              return false;
                        }

                        const auto [code_block, code_after_block] = eat_code_block.value();

                        //output_codes += std::format("layout(std140, set = 0, binding = BindlessStorageBinding) buffer {} {} _bindless{}[];", struct_typename, code_block, struct_typename);
                        //output_codes += std::format("layout(buffer_reference, scalar) buffer {} {};", struct_typename, code_block);
                        source_code = code_after_block;

                        const auto buffer_pointer_define_code = std::format("layout(buffer_reference, scalar) buffer {} {};", struct_typename, code_block);

                        const auto struct_typename_str = std::string{struct_typename};
                        if(const auto finder = _shaderBufferResourceDefineCode.find(struct_typename_str); finder != std::end(_shaderBufferResourceDefineCode)) {
                              error_message = std::format("Program::ParseMarco - Invalid statement,Resource name \"{}\" is repeat, use it directly.", struct_typename_str);
                              return false;
                        } else {
                              _shaderBufferResourceDefineCode.emplace(struct_typename_str, buffer_pointer_define_code);
                              _shaderResourceDefine.emplace(struct_typename_str, ShaderResourceInfo {ShaderResource::READ_BUFFER});
                        }

                  } else if (marco == "WBUFFER") {
                        auto struct_type_eat_word = EatWord(source_code);
                        if (!struct_type_eat_word.has_value()) {
                              error_message = std::format("Program::ParseMarco - Invalid statement, need a WBUFFER name after \"{}\" key word.", marco);
                              return false;
                        }

                        auto [struct_typename, code_after_struct_typename] = struct_type_eat_word.value(); // typename
                        source_code = code_after_struct_typename;

                        const auto eat_code_block = EatCodeBlock(source_code);
                        if (!eat_code_block.has_value()) {
                              error_message = std::format("Program::ParseMarco - Invalid statement, need a code block after WBUFFER type name \"{}\".", struct_typename);
                              return false;
                        }

                        const auto [code_block, code_after_block] = eat_code_block.value();

                        //output_codes += std::format("layout(std140, set = 0, binding = BindlessStorageBinding) buffer {} {} _bindless{}[];", struct_typename, code_block, struct_typename);
                        //output_codes += std::format("layout(buffer_reference, scalar) buffer {} {};", struct_typename, code_block);
                        source_code = code_after_block;

                        const auto buffer_pointer_define_code = std::format("layout(buffer_reference, scalar) writeonly buffer {} {};", struct_typename, code_block);

                        const auto struct_typename_str = std::string{struct_typename};
                        if(const auto finder = _shaderBufferResourceDefineCode.find(struct_typename_str); finder != std::end(_shaderBufferResourceDefineCode)) {
                              error_message = std::format("Program::ParseMarco - Invalid statement,Resource name \"{}\" is repeat, use it directly.", struct_typename_str);
                              return false;
                        } else {
                              _shaderBufferResourceDefineCode.emplace(struct_typename_str, buffer_pointer_define_code);
                              _shaderResourceDefine.emplace(struct_typename_str, ShaderResourceInfo {ShaderResource::WRITE_BUFFER});
                        }

                  } else if (marco == "SAMPLED") {
                        auto struct_type_eat_word = EatWord(source_code);
                        if (!struct_type_eat_word.has_value()) {
                              error_message = std::format("Program::ParseMarco - Invalid statement, need a SAMPELD texture name after \"{}\" key word.", marco);
                              return false;
                        }

                        auto [texture_var_name, code_after_struct_typename] = struct_type_eat_word.value(); // typename
                        source_code = code_after_struct_typename;

                        const auto texture_handle_str = std::string{texture_var_name};
                        if(const auto finder = _shaderBufferResourceDefineCode.find(texture_handle_str); finder != std::end(_shaderBufferResourceDefineCode)) {
                              error_message = std::format("Program::ParseMarco - Invalid statement,Resource name \"{}\" is repeat, use it directly.", texture_handle_str);
                              return false;
                        } else {
                              _shaderResourceDefine.emplace(texture_handle_str, ShaderResourceInfo {ShaderResource::SAMPLED_TEXTURE});
                        }

                  } else if (marco == "RWTEXTURE") {
                        auto struct_type_eat_word = EatWord(source_code);
                        if (!struct_type_eat_word.has_value()) {
                              error_message = std::format("Program::ParseMarco - Invalid statement, need a RWTEXTURE name after \"{}\" key word.", marco);
                              return false;
                        }

                        auto [texture_var_name, code_after_struct_typename] = struct_type_eat_word.value(); // typename
                        source_code = code_after_struct_typename;

                        const auto texture_handle_str = std::string{texture_var_name};
                        if(const auto finder = _shaderBufferResourceDefineCode.find(texture_handle_str); finder != std::end(_shaderBufferResourceDefineCode)) {
                              error_message = std::format("Program::ParseMarco - Invalid statement,Resource name \"{}\" is repeat, use it directly.", texture_handle_str);
                              return false;
                        } else {
                              _shaderResourceDefine.emplace(texture_handle_str, ShaderResourceInfo {ShaderResource::READ_WRITE_TEXTURE});
                        }

                  } else if (marco == "RTEXTURE") {
                        auto struct_type_eat_word = EatWord(source_code);
                        if (!struct_type_eat_word.has_value()) {
                              error_message = std::format("Program::ParseMarco - Invalid statement, need a RTEXTURE name after \"{}\" key word.", marco);
                              return false;
                        }

                        auto [texture_var_name, code_after_struct_typename] = struct_type_eat_word.value(); // typename
                        source_code = code_after_struct_typename;

                        const auto texture_handle_str = std::string{texture_var_name};
                        if(const auto finder = _shaderBufferResourceDefineCode.find(texture_handle_str); finder != std::end(_shaderBufferResourceDefineCode)) {
                              error_message = std::format("Program::ParseMarco - Invalid statement,Resource name \"{}\" is repeat, use it directly.", texture_handle_str);
                              return false;
                        } else {
                              _shaderResourceDefine.emplace(texture_handle_str, ShaderResourceInfo {ShaderResource::READ_TEXTURE});
                        }
                  } else if (marco == "WTEXTURE") {
                        auto struct_type_eat_word = EatWord(source_code);
                        if (!struct_type_eat_word.has_value()) {
                              error_message = std::format("Program::ParseMarco - Invalid statement, need a RTEXTURE name after \"{}\" key word.", marco);
                              return false;
                        }

                        auto [texture_var_name, code_after_struct_typename] = struct_type_eat_word.value(); // typename
                        source_code = code_after_struct_typename;

                        const auto texture_handle_str = std::string{texture_var_name};
                        if(const auto finder = _shaderBufferResourceDefineCode.find(texture_handle_str); finder != std::end(_shaderBufferResourceDefineCode)) {
                              error_message = std::format("Program::ParseMarco - Invalid statement,Resource name \"{}\" is repeat, use it directly.", texture_handle_str);
                              return false;
                        } else {
                              _shaderResourceDefine.emplace(texture_handle_str, ShaderResourceInfo {ShaderResource::WRITE_TEXTURE});
                        }
                  }
            }
      }

      return true;
}

bool Program::ParseSetters(std::string_view codes, entt::dense_map<std::string, std::vector<std::string>>& _setters,
std::string& output_codes, std::string& error_message, glslang_stage_t shader_type) {
      std::vector<std::string_view> lines;
      std::string_view::size_type start = 0;
      std::string_view::size_type end;

      while ((end = codes.find('\n', start)) != std::string_view::npos) {
            lines.emplace_back(codes.substr(start, end - start));
            start = end + 1;
      }

      auto EatSpace = [](const std::string_view str) -> std::pair<std::string_view, std::string_view> {
            std::string_view::size_type index = 0;
            for (; index < str.size(); index++) {
                  if (str[index] == ' ' || str[index] == '\t') { continue; } else { break; }
            }
            if (index == 0) {
                  return std::make_pair(std::string_view{}, str.substr(0, str.size()));
            } else {
                  if (index >= str.size()) {
                        return std::make_pair(str.substr(0, index), std::string_view{});
                  } else {
                        return std::make_pair(str.substr(0, index), str.substr(index, str.size() - index));
                  }
            }
      };

      auto EatWord = [](const std::string_view str) -> std::optional<std::pair<std::string_view, std::string_view>> {
            std::string_view::size_type index = 0;
            for (; index < str.size(); index++) {
                  if (str[index] == ' ' || str[index] == '\t') { break; }
            }
            if (index == 0) {
                  return std::nullopt;
            } else {
                  if (index >= str.size()) {
                        return std::make_pair(str.substr(0, index), std::string_view{});
                  } else {
                        return std::make_pair(str.substr(0, index), str.substr(index, str.size() - index));
                  }
            }
      };

      std::string entry_point_str;
      for (auto& i : ShaderTypeMap) {
            if (i.second == shader_type) {
                  entry_point_str = i.first;
                  break;
            }
      }


      for (int line = 0; line < lines.size(); ++line) {
            auto piece = lines[line];

            auto pos = piece.find("#set");
            if (pos == std::string_view::npos) {
                  if (auto epos = piece.find(entry_point_str); epos != std::string_view::npos) {
                        //replace entry_point_str to main
                        auto endpos = epos + entry_point_str.size();
                        std::string newLine = std::format("{}{}{}", piece.substr(0, epos), "main", piece.substr(endpos, piece.size() - endpos));
                        output_codes += newLine;
                        output_codes += "\n";
                  } else {
                        output_codes += piece;
                        output_codes += "\n";
                  }
                  continue;
            } else {
                  auto posCommit = piece.find("//#set");
                  if (posCommit != std::string_view::npos) {
                        output_codes += piece;
                        output_codes += "\n";
                        continue;
                  } else {
                        output_codes += "//";
                        output_codes += piece;
                        output_codes += "\n";
                  }
            }

            piece = EatSpace(piece).second;

            if (auto res = EatWord(piece); res.has_value()) {
                  if (res->first == "#set") { piece = res->second; } else {
                        error_message = std::format("Program::ParseSetters - at line {} : Invalid #set statement.",
                        line);
                        return false;
                  }
            } else {
                  error_message = std::format("Program::ParseSetters - at line {} : Invalid #set statement.", line);
                  return false;
            }

            piece = EatSpace(piece).second;

            std::pair<std::string, std::vector<std::string>> setter{};

            if (auto res = EatWord(piece); res.has_value()) {
                  if (!res->first.empty()) {
                        setter.first = res->first;
                        piece = res->second;
                  } else {
                        error_message = std::format(
                        "Program::ParseSetters - at line {} : Invalid #set statement, key is empty.", line);
                        return false;
                  }
            } else {
                  error_message = std::format(
                  "Program::ParseSetters - at line {} : Invalid #set statement, key is empty.", line);
                  return false;
            }

            piece = EatSpace(piece).second;

            if (auto res = EatWord(piece); res.has_value()) {
                  if (res->first == "=") { piece = res->second; } else {
                        error_message = std::format(
                        "Program::ParseSetters - at line {} : Invalid #set statement, missing value after key \"{}\".",
                        line, setter.first);
                        return false;
                  }
            } else {
                  error_message = std::format(
                  "Program::ParseSetters - at line {} : Invalid #set statement, missing value after key \"{}\".",
                  line, setter.first);
                  return false;
            }

            piece = EatSpace(piece).second;

            if (auto res = EatWord(piece); res.has_value()) {
                  if (!res->first.empty()) {
                        setter.second.emplace_back(res->first.begin(), res->first.end());
                        piece = res->second;
                  } else {
                        error_message = std::format(
                        "Program::ParseSetters - at line {} : Invalid #set statement, value is empty.", line);
                        return false;
                  }
            } else {
                  error_message = std::format(
                  "Program::ParseSetters - at line {} : Invalid #set statement, value is empty.", line);
                  return false;
            }

            while (true) {
                  auto v = EatSpace(piece);
                  if (v.first.empty()) {
                        break;
                  } else {
                        piece = v.second;
                        if (auto res = EatWord(piece); res.has_value()) {
                              if (!res->first.empty()) {
                                    if (res->first == "//") break;
                                    setter.second.emplace_back(res->first.begin(), res->first.end());
                                    piece = res->second;
                              }
                        }
                  }
            }

            std::string analyze_error_msg{};
            if (!AnalyzeSetter(setter, analyze_error_msg, shader_type)) {
                  error_message = analyze_error_msg;
                  return false;
            }

            auto outputS_str = std::format("set {} = ", setter.first);
            for (const auto& v : setter.second) {
                  outputS_str += v;
                  outputS_str += " ";
            }
      }
      return true;
}

void Program::ParseVS(const std::vector<uint32_t>& spv) {
      spirv_cross::Compiler comp(spv);
      spirv_cross::ShaderResources resources = comp.get_shader_resources();

      if (_autoVSInputStageBind) {
            std::vector<std::pair<uint32_t, uint32_t>> stage_input_info{}; //binding size

            for (auto& resource : resources.stage_inputs) {
                  const auto& name = comp.get_name(resource.id);
                  std::string type_name{};
                  const auto& type = comp.get_type(resource.base_type_id);
                  VkFormat input_format = VK_FORMAT_UNDEFINED;

                  const auto& binding = comp.get_decoration(resource.id, spv::Decoration::DecorationBinding);
                  const auto& location = comp.get_decoration(resource.id, spv::Decoration::DecorationLocation);
                  std::pair<uint32_t, uint32_t>* crruent_binding_info = nullptr;

                  bool missing_binding = true;
                  for (auto& i : stage_input_info) {
                        if (i.first == binding) {
                              crruent_binding_info = &i;
                              missing_binding = false;
                              break;
                        }
                  }
                  if (missing_binding) {
                        crruent_binding_info = &stage_input_info.emplace_back(binding, 0);
                  }

                  int curr_size = 0;

                  if (type.basetype == spirv_cross::SPIRType::Float && type.vecsize <= 4) {
                        switch (type.vecsize) {
                              case 1: input_format = VK_FORMAT_R32_SFLOAT;
                                    curr_size = 4;
                                    break;
                              case 2: input_format = VK_FORMAT_R32G32_SFLOAT;
                                    curr_size = 8;
                                    break;
                              case 3: input_format = VK_FORMAT_R32G32B32_SFLOAT;
                                    curr_size = 12;
                                    break;
                              case 4: input_format = VK_FORMAT_R32G32B32A32_SFLOAT;
                                    curr_size = 16;
                                    break;
                              default:
                                    {
                                          const auto err = std::format("Program::ParseVS - stage inputs has invalid vecsize: {}, at location {}, binding {}", type.vecsize, location, binding);
                                          throw std::runtime_error(err);
                                    }
                        }
                  } else if (type.basetype == spirv_cross::SPIRType::Int) {
                        input_format = VK_FORMAT_R32_SINT;
                        curr_size = 4;
                  } else if (type.basetype == spirv_cross::SPIRType::UInt) {
                        input_format = VK_FORMAT_R32_UINT;
                        curr_size = 4;
                  }

                  if (input_format == VK_FORMAT_UNDEFINED) {
                        const auto err = std::format("Program::ParseVS - stage inputs has invalid type at location {}, binding {}", location, binding);
                        throw std::runtime_error(err);
                  }

                  VkVertexInputAttributeDescription att_desc{
                        .location = location,
                        .binding = binding,
                        .format = input_format,
                        .offset = crruent_binding_info->second
                  };

                  auto str1 = std::format("\tInput: location: {}, binding: {}, name: {}, type: {}, offset {}.", location, binding, name, type_name, crruent_binding_info->second);
                  std::printf("%s\n", str1.c_str());

                  crruent_binding_info->second += curr_size;

                  _vertexInputAttributeDescription.push_back(att_desc);

                  _vertexInputStateCreateInfo.pVertexAttributeDescriptions = _vertexInputAttributeDescription.data();
                  _vertexInputStateCreateInfo.vertexAttributeDescriptionCount = _vertexInputAttributeDescription.size();
            }

            for (auto& i : stage_input_info) {
                  VkVertexInputRate rate = VK_VERTEX_INPUT_RATE_VERTEX;
                  if (_autoVSInputBindRateTable.contains(std::get<0>(i))) {
                        rate = _autoVSInputBindRateTable[std::get<0>(i)];
                  }

                  VkVertexInputBindingDescription binding_desc{
                        .binding = i.first,
                        .stride = i.second,
                        .inputRate = rate
                  };

                  auto str1 = std::format("\tInput: Binding: {}, offset {}.", i.first, i.second);
                  std::printf("%s\n", str1.c_str());

                  _vertexInputBindingDescription.push_back(binding_desc);
                  _vertexInputStateCreateInfo.pVertexBindingDescriptions = _vertexInputBindingDescription.data();
                  _vertexInputStateCreateInfo.vertexBindingDescriptionCount = _vertexInputBindingDescription.size();
            }
      }

      for (auto& resource : resources.push_constant_buffers) {
            auto& type = comp.get_type(resource.base_type_id);
            uint32_t member_count = type.member_types.size();

            for (int i = 0; i < member_count; i++) {

                  uint32_t member_size = comp.get_declared_struct_member_size(type, i);
                  uint32_t offset = comp.type_struct_member_offset(type, i);

                  const std::string& member_name = comp.get_member_name(type.self, i);
                  auto str = std::format("PushConstants: {}, offset {}, size {}", member_name, offset, member_size);
                  std::printf("%s\n", str.c_str());

                  _pushConstantDefine.emplace(member_name, PushConstantMemberInfo{offset, member_size});

                  if (i == member_count - 1) {
                        _pushConstantRange.offset = 0;
                        _pushConstantRange.size = offset + member_size;
                  }
            }
      }
}

void Program::ParseFS(const std::vector<uint32_t>& spv) {
      spirv_cross::Compiler comp(spv);
      spirv_cross::ShaderResources resources = comp.get_shader_resources();

      auto output_target_count = resources.stage_outputs.size();

      for (auto& resource : resources.stage_outputs) {
            const auto& name = comp.get_name(resource.id);
            const auto& type_id = comp.get_name(resource.base_type_id);

            auto str1 = std::format("\tOutPutTarget: name {}, type {}.", name, type_id);
            std::printf("%s\n", str1.c_str());
      }

      if (_colorBlendAttachmentState.size() != output_target_count) {
            const auto err = std::format("Program::ParseFS - Output target count mismatch, expected {}, got {}, please add or move #set color_blend in shader source code.", output_target_count,
            _colorBlendAttachmentState.size());
            throw std::runtime_error(err);
      }

      for (auto& resource : resources.push_constant_buffers) {
            auto& type = comp.get_type(resource.base_type_id);
            uint32_t member_count = type.member_types.size();

            for (int i = 0; i < member_count; i++) {

                  size_t member_size = comp.get_declared_struct_member_size(type, i);
                  size_t offset = comp.type_struct_member_offset(type, i);

                  // const std::string& member_name = comp.get_member_name(type.self, i);
                  // auto str = std::format("PushConstants: {}, offset {}, size {}", member_name, offset, member_size);
                  // std::printf("%s\n", str.c_str());

                  if (i == member_count - 1) {
                        uint32_t ps_size = offset + member_size;
                        if (ps_size != _pushConstantRange.size) {
                              const auto err = std::format("Program::ParseFS - fs's struct not match with vs, please copy it from vs to fs, expected {}, got {}.", _pushConstantRange.size, ps_size);
                              throw std::runtime_error(err);
                        }
                  }
            }
      }
}

void Program::ParseCS(const std::vector<uint32_t>& spv) {
      spirv_cross::Compiler comp(spv);
      spirv_cross::ShaderResources resources = comp.get_shader_resources();

      for (auto& resource : resources.push_constant_buffers) {
            auto& type = comp.get_type(resource.base_type_id);
            uint32_t member_count = type.member_types.size();

            for (int i = 0; i < member_count; i++) {
                  auto& member_type = comp.get_type(type.member_types[i]);
                  const std::string& member_type_name = comp.get_name(member_type.self);

                  uint32_t member_size = comp.get_declared_struct_member_size(type, i);
                  uint32_t offset = comp.type_struct_member_offset(type, i);

                  // const std::string& member_name = comp.get_member_name(type.self, i);
                  // auto str = std::format("PushConstants: {}, offset {}, size {}", member_name, offset, member_size);
                  // std::printf("%s\n", str.c_str());

                  _pushConstantDefine.emplace(member_type_name, PushConstantMemberInfo{offset, member_size});

                  if (i == member_count - 1) {
                        _pushConstantRange.offset = 0;
                        _pushConstantRange.size = offset + member_size;
                  }
            }
      }
}

bool Program::AnalyzeSetter(const std::pair<std::string, std::vector<std::string>>& setter, std::string& error_msg, glslang_stage_t shader_type) {
      static std::unordered_map<std::string, std::unordered_map<std::string, uint64_t>> SetterKeyValueMapper{
            {
                  "topology", {
                        {"point_list", VK_PRIMITIVE_TOPOLOGY_POINT_LIST},
                        {"line_list", VK_PRIMITIVE_TOPOLOGY_LINE_LIST},
                        {"line_strip", VK_PRIMITIVE_TOPOLOGY_LINE_STRIP},
                        {"triangle_list", VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST},
                        {"triangle_strip", VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP},
                        {"triangle_fan", VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN},
                        {"line_list_with_adjacency", VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY},
                        {"line_strip_with_adjacency", VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY},
                        {"triangle_list_with_adjacency", VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY},
                        {"triangle_strip_with_adjacency", VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY},
                        {"patch_list", VK_PRIMITIVE_TOPOLOGY_PATCH_LIST},
                  }
            },
            {
                  "polygon_mode", {
                        {"fill", VK_POLYGON_MODE_FILL},
                        {"line", VK_POLYGON_MODE_LINE},
                        {"point", VK_POLYGON_MODE_POINT},
                  }
            },
            {
                  "cull_mode", {
                        {"front", VK_CULL_MODE_FRONT_BIT},
                        {"back", VK_CULL_MODE_BACK_BIT},
                        {"none", VK_CULL_MODE_NONE},
                        {"front_and_back", VK_CULL_MODE_FRONT_AND_BACK},
                  }
            },
            {
                  "front_face", {
                        {"clockwise", VK_FRONT_FACE_CLOCKWISE},
                        {"counter_clockwise", VK_FRONT_FACE_COUNTER_CLOCKWISE},
                  }
            },
            {
                  "depth_write", {
                        {"true", VK_TRUE},
                        {"false", VK_FALSE},
                  }
            },
            {
                  "depth_test", {
                        {"default", VK_COMPARE_OP_LESS_OR_EQUAL},
                        {"never", VK_COMPARE_OP_NEVER},
                        {"less", VK_COMPARE_OP_LESS},
                        {"equal", VK_COMPARE_OP_EQUAL},
                        {"less_or_equal", VK_COMPARE_OP_LESS_OR_EQUAL},
                        {"greater", VK_COMPARE_OP_GREATER},
                        {"not_equal", VK_COMPARE_OP_NOT_EQUAL},
                        {"greater_or_equal", VK_COMPARE_OP_GREATER_OR_EQUAL},
                        {"always", VK_COMPARE_OP_ALWAYS},
                  }
            }
      };

      static std::unordered_map<glslang_stage_t, std::set<std::string>> SetterAvailableInShaderType{
            {
                  glslang_stage_t::GLSLANG_STAGE_VERTEX, {
                        "vs_location",
                        "vs_binding",
                        "topology",
                        "polygon_mode",
                        "cull_mode",
                        "front_face",

                        "depth_write",
                        "depth_test",
                        "depth_bias",
                        "depth_bounds_test",
                        "line_width"
                  }
            },

            {
                  glslang_stage_t::GLSLANG_STAGE_FRAGMENT, {
                        "rt",
                        "ds",
                        "color_blend",

                        "depth_write",
                        "depth_test",
                        "depth_bias",
                        "depth_bounds_test",
                        "line_width"
                  }
            }
      };

      auto CheckAndFetchValue = [&](const std::string& the_key, const std::string& the_value,
      std::string& error) -> std::optional<uint64_t> {
            if (const auto it = SetterKeyValueMapper.find(the_key); it != SetterKeyValueMapper.end()) {
                  if (const auto it2 = it->second.find(the_value); it2 != it->second.end()) {
                        return it2->second;
                  } else {
                        error = std::format(R"(Invalid value "{}" for key "{}", )", the_value, the_key);
                        error += std::format("Valid values are : \n\t\t");
                        for (const auto& k : it->second | std::views::keys) {
                              error += k;
                              error += " ";
                        }

                        return std::nullopt;
                  }
            } else {
                  error = std::format("Invalid setter key \"{}\" .", the_key);
                  return std::nullopt;
            }
      };

      auto Analyze = [&]<typename T0>(T0& input, const std::string& key, const std::string& the_value,
      std::string& error) -> bool {
            if (const auto res = CheckAndFetchValue(key, the_value, error_msg); res.has_value()) {
                  input = *((T0*)&res.value());
                  return true;
            }
            return false;
      };

      auto ErrorArgumentUnmatching = [](const std::string& key, uint32_t argument_target, uint32_t error_argument, std::string& error_msg, const std::string& argument_format) -> bool {
            error_msg = std::format("Invalid argument count for key \"{}\". Expected {}, got {}, format: [{}].",
            key, argument_target, error_argument, argument_format);
            return false;
      };

      auto ErrorArgument = [](const std::string& key, uint32_t err_target, const std::string& value, std::string& error_msg, const std::string& vaild_values) -> bool {
            error_msg = std::format("Invalid argument {} for key \"{}\". Expected [{}], got \"{}\".", key, err_target, vaild_values, value);
            return false;
      };

      auto ErrorSetInShaderType = [](const std::string& key, glslang_stage_t shader_type, std::string& error_msg) -> bool {
            error_msg = std::format("Invalid setter key \"{}\" for shader type \"{}\".", key, HelperShaderStageToString(shader_type));
            return false;
      };

      const auto& key = setter.first;
      const auto& values = setter.second;

      auto vaild_setters = SetterAvailableInShaderType.find(shader_type);
      if (vaild_setters == SetterAvailableInShaderType.end()) {
            //shouldn't be here
            return ErrorSetInShaderType(key, shader_type, error_msg);
      }

      if (!vaild_setters->second.contains(key)) {
            error_msg = std::format("Invalid setter key \"{}\" for shader type \"{}\".", key, HelperShaderStageToString(shader_type));
            return false;
      }


      if (key == "topology") {
            if (!Analyze(_inputAssemblyStateCreateInfo.topology, key, values[0], error_msg)) return false;
      } else if (key == "polygon_mode") {
            if (!Analyze(_rasterizationStateCreateInfo.polygonMode, key, values[0], error_msg)) return false;
      } else if (key == "cull_mode") {
            if (!Analyze(_rasterizationStateCreateInfo.cullMode, key, values[0], error_msg)) return false;
      } else if (key == "front_face") {
            if (!Analyze(_rasterizationStateCreateInfo.frontFace, key, values[0], error_msg)) return false;
      } else if (key == "depth_write") {
            if (!Analyze(_depthStencilStateCreateInfo.depthWriteEnable, key, values[0], error_msg)) return false;
      } else if (key == "depth_test") {
            _depthStencilStateCreateInfo.depthTestEnable = true;
            if (!Analyze(_depthStencilStateCreateInfo.depthCompareOp, key, values[0], error_msg)) return false;
      } else if (key == "vs_location") {
            _autoVSInputStageBind = false;
            if (values.size() == 4) {
                  // bind, location, format, offset
                  uint32_t binding;
                  uint32_t location;
                  uint32_t offset;
                  try {
                        binding = std::stoi(values[0]);
                  } catch (std::runtime_error&) {
                        error_msg = std::format("Invalid argument 1 for key \"{}\". Expected integer, got \"{}\".", key, values[0]);
                        return false;
                  }

                  try {
                        location = std::stoi(values[1]);
                  } catch (std::runtime_error&) {
                        error_msg = std::format("Invalid argument 2 for key \"{}\". Expected integer, got \"{}\".", key, values[1]);
                        return false;
                  }

                  try {
                        offset = std::stoi(values[3]);
                  } catch (std::runtime_error&) {
                        error_msg = std::format("Invalid argument 4 for key \"{}\". Expected integer, got \"{}\".", key, values[3]);
                        return false;
                  }

                  VkFormat format = GetVkFormatFromStringSimpled(values[2]);

                  if (format == VK_FORMAT_UNDEFINED) {
                        return ErrorArgument(key, 3, values[2], error_msg, "(format)");
                  }

                  VkVertexInputAttributeDescription att_desc{
                        .location = location,
                        .binding = binding,
                        .format = format,
                        .offset = offset
                  };

                  _vertexInputAttributeDescription.push_back(att_desc);

                  _vertexInputStateCreateInfo.pVertexAttributeDescriptions = _vertexInputAttributeDescription.data();
                  _vertexInputStateCreateInfo.vertexAttributeDescriptionCount = _vertexInputAttributeDescription.size();
            } else {
                  return ErrorArgumentUnmatching(key, 4, values.size(), error_msg, "bind, location , format, offset");
            }
      } else if (key == "vs_binding") {
            if (values.size() == 3) {
                  _autoVSInputStageBind = false;
                  // binding, stride_size, binding_rate
                  uint32_t binding;
                  uint32_t stride_size;

                  try {
                        binding = std::stoi(values[0]);
                  } catch (std::runtime_error&) {
                        error_msg = std::format("Invalid argument 1 for key \"{}\". Expected integer, got \"{}\".", key, values[0]);
                        return false;
                  }

                  try {
                        stride_size = std::stoi(values[1]);
                  } catch (std::runtime_error&) {
                        error_msg = std::format("Invalid argument 2 for key \"{}\". Expected integer, got \"{}\".", key, values[1]);
                        return false;
                  }

                  VkVertexInputBindingDescription binding_desc{
                        .binding = binding,
                        .stride = stride_size,
                        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
                  };

                  if (values[2] == "vertex") {
                        binding_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
                  } else if (values[2] == "instance") {
                        binding_desc.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
                  } else {
                        return ErrorArgument(key, 3, values[2], error_msg, "vertex, instance");
                  }
                  _vertexInputBindingDescription.push_back(binding_desc);
                  _vertexInputStateCreateInfo.pVertexBindingDescriptions = _vertexInputBindingDescription.data();
                  _vertexInputStateCreateInfo.vertexBindingDescriptionCount = _vertexInputBindingDescription.size();
            }
            if (values.size() == 2) {
                  if (!_autoVSInputStageBind) {
                        error_msg = "Auto vertex input stage bind is disabled, please disable it by adding #set vs_binding = [binding, stride_size, binding_rate] in shader source code.";
                        return false;
                  }

                  uint32_t binding;

                  try {
                        binding = std::stoi(values[0]);
                  } catch (std::runtime_error&) {
                        error_msg = std::format("Invalid argument 1 for key \"{}\". Expected integer, got \"{}\".", key, values[0]);
                        return false;
                  }

                  if (values[2] == "vertex") {
                        _autoVSInputBindRateTable[binding] = VK_VERTEX_INPUT_RATE_VERTEX;
                  } else if (values[2] == "instance") {
                        _autoVSInputBindRateTable[binding] = VK_VERTEX_INPUT_RATE_INSTANCE;
                  } else {
                        return ErrorArgument(key, 3, values[2], error_msg, "vertex, instance");
                  }
            } else {
                  return ErrorArgumentUnmatching(key, 4, values.size(), error_msg, "binding, stride_size, binding_rate");
            }
      } else if (key == "depth_bias") {
            if (values.size() == 3) {
                  // depthBiasConstantFactor, depthBiasClamp, depthBiasSlopeFactor
                  float depthBiasConstantFactor;
                  float depthBiasClamp;
                  float depthBiasSlopeFactor;

                  try {
                        depthBiasConstantFactor = std::stof(values[0]);
                  } catch (std::runtime_error&) {
                        error_msg = std::format("Invalid argument 1 for key \"{}\". Expected float, got \"{}\".", key, values[0]);
                        return false;
                  }

                  try {
                        depthBiasClamp = std::stof(values[1]);
                  } catch (std::runtime_error&) {
                        error_msg = std::format("Invalid argument 2 for key \"{}\". Expected float, got \"{}\".", key, values[1]);
                        return false;
                  }

                  try {
                        depthBiasSlopeFactor = std::stof(values[2]);
                  } catch (std::runtime_error&) {
                        error_msg = std::format("Invalid argument 3 for key \"{}\". Expected float, got \"{}\".", key, values[2]);
                        return false;
                  }

                  _rasterizationStateCreateInfo.depthBiasEnable = true;
                  _rasterizationStateCreateInfo.depthBiasConstantFactor = depthBiasConstantFactor;
                  _rasterizationStateCreateInfo.depthBiasClamp = depthBiasClamp;
                  _rasterizationStateCreateInfo.depthBiasSlopeFactor = depthBiasSlopeFactor;
            } else {
                  return ErrorArgumentUnmatching(key, 3, values.size(), error_msg, "depth_bias_constant_factor, depth_bias_clamp, depth_bias_slope_factor");
            }
      } else if (key == "depth_bounds_test") {
            if (values.size() == 2) {
                  // min_depth, max_depth
                  float min_depth;
                  float max_depth;

                  try {
                        min_depth = std::stof(values[0]);
                  } catch (std::runtime_error&) {
                        error_msg = std::format("Invalid argument 1 for key \"{}\". Expected float, got \"{}\".", key, values[0]);
                        return false;
                  }

                  try {
                        max_depth = std::stof(values[1]);
                  } catch (std::runtime_error&) {
                        error_msg = std::format("Invalid argument 2 for key \"{}\". Expected float, got \"{}\".", key, values[1]);
                        return false;
                  }

                  _depthStencilStateCreateInfo.depthBoundsTestEnable = true;
                  _depthStencilStateCreateInfo.minDepthBounds = min_depth;
                  _depthStencilStateCreateInfo.maxDepthBounds = max_depth;
            } else {
                  return ErrorArgumentUnmatching(key, 2, values.size(), error_msg, "min_depth, max_depth");
            }
      } else if (key == "line_width") {
            float line_width;

            try {
                  line_width = std::stof(values[0]);
            } catch (std::runtime_error&) {
                  error_msg = std::format("Invalid argument 1 for key \"{}\". Expected float, got \"{}\".", key, values[0]);
                  return false;
            }
            _rasterizationStateCreateInfo.lineWidth = line_width;
      } else if (key == "color_blend") {
            if (values.size() == 1) {
                  // blend_enable
                  if (values[0] == "false") {
                        VkPipelineColorBlendAttachmentState default_profile{};
                        default_profile.blendEnable = VK_FALSE;
                        default_profile.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
                        default_profile.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
                        default_profile.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
                        default_profile.colorBlendOp = VK_BLEND_OP_ADD;
                        default_profile.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                        default_profile.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
                        default_profile.alphaBlendOp = VK_BLEND_OP_ADD;
                        _colorBlendAttachmentState.push_back(default_profile);
                  } else if (values[0] == "add") {
                        VkPipelineColorBlendAttachmentState additive_blend = {};
                        additive_blend.blendEnable = VK_TRUE;
                        additive_blend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
                        additive_blend.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
                        additive_blend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
                        additive_blend.colorBlendOp = VK_BLEND_OP_ADD;
                        additive_blend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                        additive_blend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                        additive_blend.alphaBlendOp = VK_BLEND_OP_ADD;
                        _colorBlendAttachmentState.push_back(additive_blend);
                  } else if (values[0] == "sub") {
                        VkPipelineColorBlendAttachmentState subtractive_blend = {};
                        subtractive_blend.blendEnable = VK_TRUE;
                        subtractive_blend.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
                        subtractive_blend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
                        subtractive_blend.colorBlendOp = VK_BLEND_OP_SUBTRACT;
                        subtractive_blend.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
                        subtractive_blend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                        subtractive_blend.alphaBlendOp = VK_BLEND_OP_SUBTRACT;
                        subtractive_blend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
                        _colorBlendAttachmentState.push_back(subtractive_blend);
                  } else if (values[0] == "alpha") {
                        VkPipelineColorBlendAttachmentState alpha_blend = {};
                        alpha_blend.blendEnable = VK_TRUE;
                        alpha_blend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
                        alpha_blend.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
                        alpha_blend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                        alpha_blend.colorBlendOp = VK_BLEND_OP_ADD;
                        alpha_blend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                        alpha_blend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
                        alpha_blend.alphaBlendOp = VK_BLEND_OP_ADD;
                        _colorBlendAttachmentState.push_back(alpha_blend);
                  } else if (values[0] == "min") {
                        VkPipelineColorBlendAttachmentState darken_blend = {};
                        darken_blend.blendEnable = VK_TRUE;
                        darken_blend.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
                        darken_blend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
                        darken_blend.colorBlendOp = VK_BLEND_OP_MIN;
                        darken_blend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                        darken_blend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                        darken_blend.alphaBlendOp = VK_BLEND_OP_MIN;
                        darken_blend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
                        _colorBlendAttachmentState.push_back(darken_blend);
                  } else if (values[0] == "max") {
                        VkPipelineColorBlendAttachmentState lighten_blend = {};
                        lighten_blend.blendEnable = VK_TRUE;
                        lighten_blend.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
                        lighten_blend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
                        lighten_blend.colorBlendOp = VK_BLEND_OP_MAX;
                        lighten_blend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                        lighten_blend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                        lighten_blend.alphaBlendOp = VK_BLEND_OP_MAX;
                        lighten_blend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
                        _colorBlendAttachmentState.push_back(lighten_blend);
                  } else if (values[0] == "mul") {
                        VkPipelineColorBlendAttachmentState blend = {};
                        blend.blendEnable = VK_TRUE;
                        blend.srcColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR;
                        blend.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
                        blend.colorBlendOp = VK_BLEND_OP_ADD;
                        blend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                        blend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
                        blend.alphaBlendOp = VK_BLEND_OP_ADD;
                        blend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
                        _colorBlendAttachmentState.push_back(blend);
                  } else if (values[0] == "screen") {
                        VkPipelineColorBlendAttachmentState blend = {};
                        blend.blendEnable = VK_TRUE;
                        blend.srcColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
                        blend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
                        blend.colorBlendOp = VK_BLEND_OP_ADD;
                        blend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                        blend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
                        blend.alphaBlendOp = VK_BLEND_OP_ADD;
                        blend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
                        _colorBlendAttachmentState.push_back(blend);
                  } else {
                        return ErrorArgument(key, 1, values[0], error_msg, "(false) or (add, sub, alpha, min, max, mul, screen)");
                        //return ErrorArgumentUnmatching(key, 1, values.size(), error_msg, "if you enable color blend, please use argument: (defualt) or (colorWriteMask, dstAlphaBlendFactor, srcAlphaBlendFactor, colorBlendOp, alphaBlendOp)");
                  }

                  _colorBlendStateCreateInfo.attachmentCount = _colorBlendAttachmentState.size();
                  _colorBlendStateCreateInfo.pAttachments = _colorBlendAttachmentState.data();
            } else {
                  return ErrorArgumentUnmatching(key, 1, values.size(), error_msg, "(blend_enable) or (colorWriteMask, dstAlphaBlendFactor, srcAlphaBlendFactor, colorBlendOp, alphaBlendOp)");
            }
      } else if (key == "rt") {
            for (int idx = 0; idx < values.size(); idx++) {
                  VkFormat vk_format = GetVkFormatFromStringSimpled(values[idx]);

                  if (vk_format == VK_FORMAT_UNDEFINED) {
                        error_msg = std::format("Invalid argument {} for key \"{}\". Expected a vaild format, got \"{}\".", idx, key, values[idx]);
                        return false;
                  }

                  _renderTargetFormat.push_back(vk_format);
            }
            _renderingCreateInfo.colorAttachmentCount = _renderTargetFormat.size();
            _renderingCreateInfo.pColorAttachmentFormats = _renderTargetFormat.data();
      } else if (key == "ds") {
            if (values.size() == 1) {
                  VkFormat vk_format = GetVkFormatFromStringSimpled(values[0]);
                  if (IsDepthStencilOnlyFormat(vk_format)) {
                        _renderingCreateInfo.depthAttachmentFormat = vk_format;
                        _renderingCreateInfo.stencilAttachmentFormat = vk_format;
                  } else if (IsDepthOnlyFormat(vk_format)) {
                        _renderingCreateInfo.depthAttachmentFormat = vk_format;
                        _renderingCreateInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;
                  } else {
                        error_msg = std::format(
                        "Invalid argument {} for key \"{}\". Expected a vaild format, got \"{}\", vaild format:[d16_unorm_s8_uint, d24_unorm_s8_uint, d32_sfloat_s8_uint, d16_unorm, d32_sfloat].",
                        0, key, values[0]);
                        return false;
                  }
            } else {
                  error_msg = std::format(
                  "Invalid argument count for key \"{}\". Expected 1 , got {}, format: (depth_(stencil)_format, vaild format:[d16_unorm_s8_uint, d24_unorm_s8_uint, d32_sfloat_s8_uint, d16_unorm, d32_sfloat]).",
                  key, values.size());
                  return false;
            }
      } else {
            error_msg = std::format("Invalid setter key \"{}\" .", key);
            return false;
      }

      return true;
}

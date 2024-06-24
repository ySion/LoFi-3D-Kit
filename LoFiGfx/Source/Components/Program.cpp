#define NOMINMAX
#include "Program.h"

#include "../Context.h"
#include "../Message.h"

#include "../Third/dxc/dxcapi.h"
#include "../Third/spirv-cross/spirv_cross.hpp"
#include "../Third/spirv-cross/spirv_hlsl.hpp"

#include "SDL3/SDL.h"

#include <wrl/client.h>

using namespace LoFi::Component;

template<class T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

Program::~Program() {
      if (_vs != VK_NULL_HANDLE) {
            vkDestroyShaderModule(volkGetLoadedDevice(), _vs, nullptr);
      }
      if (_ps != VK_NULL_HANDLE) {
            vkDestroyShaderModule(volkGetLoadedDevice(), _ps, nullptr);
      }
      if (_cs != VK_NULL_HANDLE) {
            vkDestroyShaderModule(volkGetLoadedDevice(), _cs, nullptr);
      }
      if (_ms != VK_NULL_HANDLE) {
            vkDestroyShaderModule(volkGetLoadedDevice(), _ms, nullptr);
      }
      if (_as != VK_NULL_HANDLE) {
            vkDestroyShaderModule(volkGetLoadedDevice(), _as, nullptr);
      }
}

Program::Program(entt::entity id) : _id(id) {
}

bool Program::CompileFromSourceCode(std::string_view name, std::string_view source) {
      std::string backup_name = _programName;
      std::string backup_code = _codes;

      _programName = name;

      //Init Pipeline CIs, use default profile

      _inputAssemblyStateCreateInfo_temp = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .primitiveRestartEnable = VK_FALSE
      };

      _rasterizationStateCreateInfo_temp = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .depthClampEnable = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .cullMode = VK_CULL_MODE_BACK_BIT,
            .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
            .depthBiasEnable = VK_FALSE,
            .depthBiasConstantFactor = 0.0f,
            .depthBiasClamp = 0.0f,
            .depthBiasSlopeFactor = 0.0f,
            .lineWidth = 1.0f
      };

      _vertexInputStateCreateInfo_temp = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .vertexBindingDescriptionCount = 0,
            .pVertexBindingDescriptions = nullptr,
            .vertexAttributeDescriptionCount = 0,
            .pVertexAttributeDescriptions = nullptr
      };

      _depthStencilStateCreateInfo_temp = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .depthTestEnable = VK_TRUE,
            .depthWriteEnable = VK_TRUE,
            .depthCompareOp = VK_COMPARE_OP_LESS,
            .depthBoundsTestEnable = VK_FALSE,
            .stencilTestEnable = VK_FALSE,
            .front = {},
            .back = {},
            .minDepthBounds = 0.0f,
            .maxDepthBounds = 1.0f
      };

      _colorBlendStateCreateInfo_temp = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .logicOpEnable = VK_FALSE,
            .logicOp = VK_LOGIC_OP_COPY,
            .attachmentCount = 0,
            .pAttachments = nullptr,
            .blendConstants = {0.0f, 0.0f, 0.0f, 0.0f}
      };

      _pushConstantRange_temp = {
            .stageFlags = VK_SHADER_STAGE_ALL,
            .offset = 0,
            .size = 0
      };

      std::vector<uint32_t> spv[5]{};

      entt::dense_map<std::string, std::vector<std::string> > setters{};

      std::string setter_parse_err_msg{};
      std::string source_code_output{};

      if (!ParseSetters(source, setters, source_code_output, setter_parse_err_msg)) {
            auto str = std::format(
                  "Program::CompileFromSourceCode - Failed to parse setters for shader program \"{}\" -- Fail\n Message:\n{}",
                  _programName, setter_parse_err_msg);
            MessageManager::Log(MessageType::Warning, str);
            _programName = backup_name;
            _codes = backup_code;
            return false;
      }

      _codes = source_code_output;

      std::vector<std::string> success_shaders{};

      if (_codes.find("VSMain") != std::string::npos) {
            if (!CompileFromCode(L"VSMain", L"vs_6_6", spv[0])) {
                  _programName = backup_name;
                  _codes = backup_code;
                  return false;
            }
            success_shaders.emplace_back("Vertex");
      }

      if (_codes.find("PSMain") != std::string::npos) {
            if (!CompileFromCode(L"PSMain", L"ps_6_6", spv[1])) {
                  _programName = backup_name;
                  _codes = backup_code;
                  return false;
            }
            success_shaders.emplace_back("Pixel");
      }

      if (_codes.find("CSMain") != std::string::npos) {
            if (!CompileFromCode(L"CSMain", L"cs_6_6", spv[2])) {
                  _programName = backup_name;
                  _codes = backup_code;
                  return false;
            }
            success_shaders.emplace_back("Compute");
      }

      if (_codes.find("MSMain") != std::string::npos) {
            if (!CompileFromCode(L"MSMain", L"ms_6_6", spv[3])) {
                  _programName = backup_name;
                  _codes = backup_code;
                  return false;
            }
            success_shaders.emplace_back("Mesh");
      }

      if (_codes.find("ASMain") != std::string::npos) {
            if (!CompileFromCode(L"ASMain", L"as_6_6", spv[4])) {
                  _programName = backup_name;
                  _codes = backup_code;
                  return false;
            }
            success_shaders.emplace_back("Task");
      }
      auto str = std::format("Program::CompileFromSourceCode - Compiled Shader Program \"{}\"  -- Success\n",
                             _programName);

      for (const auto &success_shader: success_shaders) {
            str += std::format("\t\t{} Shader -- SUCCESS \n", success_shader);
      }
      //start parse

      MessageManager::Log(MessageType::Normal, str);

      _spvVS = std::move(spv[0]);
      _spvPS = std::move(spv[1]);
      _spvCS = std::move(spv[2]);
      _spvMS = std::move(spv[3]);
      _spvAS = std::move(spv[4]);

      bool parseSuccess = true;
      for (const auto &success_shader: success_shaders) {
            if (success_shader == "Vertex") {
                  parseSuccess = parseSuccess && ParseVS();
            } else if (success_shader == "Pixel") {
                  parseSuccess = parseSuccess && ParsePS();
            }
      }

      if (parseSuccess) {
            VkShaderModuleCreateInfo shader_ci{
                  .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                  .pNext = nullptr,
                  .flags = 0,
                  .codeSize = 0,
                  .pCode = nullptr
            };

            std::vector<std::pair<VkShaderModule *, VkShaderModule> > compile_replace = {};

            VkShaderModule *compile_target = nullptr;
            bool module_success = true;
            for (const auto &success_shader: success_shaders) {
                  if (success_shader == "Vertex") {
                        shader_ci.codeSize = _spvVS.size() * sizeof(uint32_t);
                        shader_ci.pCode = _spvVS.data();
                        compile_target = &_vs;
                  } else if (success_shader == "Pixel") {
                        shader_ci.codeSize = _spvPS.size() * sizeof(uint32_t);
                        shader_ci.pCode = _spvPS.data();
                        compile_target = &_ps;
                  } else if (success_shader == "Compute") {
                        shader_ci.codeSize = _spvCS.size() * sizeof(uint32_t);
                        shader_ci.pCode = _spvCS.data();
                        compile_target = &_cs;
                  } else if (success_shader == "Mesh") {
                        shader_ci.codeSize = _spvMS.size() * sizeof(uint32_t);
                        shader_ci.pCode = _spvMS.data();
                        compile_target = &_ms;
                  } else if (success_shader == "Task") {
                        shader_ci.codeSize = _spvAS.size() * sizeof(uint32_t);
                        shader_ci.pCode = _spvAS.data();
                        compile_target = &_as;
                  }

                  if (compile_target == nullptr) {
                        //shouldn't be here
                        auto str = std::format(
                              "Program::CompileFromSourceCode - Failed to find shader module \"{}\" at \"{}\" -- Fail\n",
                              _programName, success_shader);
                        MessageManager::Log(MessageType::Warning, str);
                        module_success = false;
                        break;
                  }

                  VkShaderModule shader_module{};
                  if (auto res = vkCreateShaderModule(volkGetLoadedDevice(), &shader_ci, nullptr, &shader_module);
                        res != VK_SUCCESS) {
                        auto str = std::format(
                              "Program::CompileFromSourceCode - Failed to create shader module \"{}\" at \"{}\" -- Fail\n",
                              _programName, success_shader);
                        MessageManager::Log(MessageType::Warning, str);
                        module_success = false;
                        break;
                  } else {
                        compile_replace.push_back({compile_target, shader_module});
                  }
            }

            if (module_success) {
                  // 成功后统一替换
                  for (auto &[target, module]: compile_replace) {
                        if (*target != VK_NULL_HANDLE) {
                              vkDestroyShaderModule(volkGetLoadedDevice(), *target, nullptr);
                        }
                        *target = module;
                  }
            } else {
                  _programName = backup_name;
                  _codes = backup_code;
                  return false;
            }
      } else {
            auto str = std::format("Program::CompileFromSourceCode - Failed to parse shader program \"{}\" -- Fail\n",
                                   _programName);
            MessageManager::Log(MessageType::Warning, str);
            _programName = backup_name;
            _codes = backup_code;
            return false;
      }

      //success

      _colorBlendAttachmentState_prepared = _colorBlendAttachmentState_temp;
      _inputAssemblyStateCreateInfo_prepared = _inputAssemblyStateCreateInfo_temp;
      _rasterizationStateCreateInfo_prepared = _rasterizationStateCreateInfo_temp;

      _vertexInputAttributeDescription_prepared = std::move(_vertexInputAttributeDescription_temp);
      _vertexInputBindingDescription_prepared = std::move(_vertexInputBindingDescription_temp);
      _vertexInputStateCreateInfo_prepared = _vertexInputStateCreateInfo_temp;

      _depthStencilStateCreateInfo_prepared = _depthStencilStateCreateInfo_temp;

      _colorBlendAttachmentState_prepared = std::move(_colorBlendAttachmentState_temp);
      _colorBlendStateCreateInfo_prepared = _colorBlendStateCreateInfo_temp;

      _renderTargetFormat_prepared = std::move(_renderTargetFormat_temp);
      _renderingCreateInfo_prepared = _renderingCreateInfo_temp;

      _pushConstantRange_prepared = _pushConstantRange_temp;

      return true;
}

bool Program::CompileFromCode(const wchar_t *entry_point, const wchar_t *shader_type, std::vector<uint32_t> &spv) {
      auto &compiler = *(LoFi::Context::Get()->_dxcCompiler);

      DxcBuffer buffer = {
            .Ptr = _codes.data(),
            .Size = (uint32_t) _codes.size(),
            .Encoding = 0
      };

      std::vector<LPCWSTR> args{};
      args.push_back(L"-Zpc");
      args.push_back(L"-O0");

      args.push_back(L"-HV");
      args.push_back(L"2021");

      args.push_back(L"-T");
      args.push_back(shader_type);

      args.push_back(L"-E");
      args.push_back(entry_point);

      args.push_back(L"-auto-binding-space");
      args.push_back(L"0");

      args.push_back(L"-spirv");
      args.push_back(L"-fspv-reflect");
      args.push_back(L"-fspv-target-env=vulkan1.3");


      ComPtr<IDxcResult> dxc_result{};
      compiler.Compile(&buffer, args.data(), (uint32_t) (args.size()), nullptr, IID_PPV_ARGS(&dxc_result));

      HRESULT result_code = E_FAIL;
      dxc_result->GetStatus(&result_code);
      ComPtr<IDxcBlobEncoding> dxc_error = nullptr;
      dxc_result->GetErrorBuffer(&dxc_error);


      if (FAILED(result_code)) {
            // compile error
            bool const has_errors = (dxc_error != nullptr && dxc_error->GetBufferPointer() != nullptr);
            auto error_msg = std::format(
                  "Program::CompileFromCode - Failed to compile shader \"{}\" for entry point \"{}\":\n{}",
                  _programName, (const char *) entry_point,
                  has_errors ? (char const *) dxc_error->GetBufferPointer() : "");
            MessageManager::Log(MessageType::Warning, error_msg);
            return false;
      }

      if (dxc_error != nullptr) {
            // has_warnings
            if (dxc_error->GetBufferPointer() != nullptr) {
                  auto warning_msg = std::format(
                        "Program::CompileFromCode - Compiled shader \"{}\" for entry point \"{}\" with Warning(s):\n{}",
                        _programName, (const char *) entry_point, (char const *) dxc_error->GetBufferPointer());
                  MessageManager::Log(MessageType::Warning, warning_msg);
            }
      }


      ComPtr<IDxcBlob> spv_object{};
      dxc_result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&spv_object), nullptr);

      if (spv_object->GetBufferSize() % sizeof(uint32_t) != 0) {
            auto str = std::format(
                  R"(Program::CompileFromCode - Invalid SPIR-V size shader compile "{}" for entry point "{}". Invalid size: "{}")",
                  _programName, (const char *) entry_point, spv_object->GetBufferSize());
            MessageManager::Log(MessageType::Warning, str);
            return false;
      }

      spv.resize(spv_object->GetBufferSize() / sizeof(uint32_t));
      memcpy(spv.data(), spv_object->GetBufferPointer(), spv_object->GetBufferSize());

      return true;
}

bool Program::ParseSetters(std::string_view codes, entt::dense_map<std::string, std::vector<std::string> > &_setters,
                           std::string &output_codes, std::string &error_message) {
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

      auto EatWord = [](const std::string_view str) -> std::optional<std::pair<std::string_view, std::string_view> > {
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

      for (int line = 0; line < lines.size(); ++line) {
            auto piece = lines[line];

            auto pos = piece.find("#set");
            if (pos == std::string_view::npos) {
                  output_codes += piece;
                  output_codes += "\n";
                  continue;
            } else {
                  auto posCommit = piece.find("//#set");
                  if (posCommit != std::string_view::npos) {
                        continue;
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

            std::pair<std::string, std::vector<std::string> > setter{};

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
                        setter.second.emplace_back(std::string(res->first.begin(), res->first.end()));
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
                                    if(res->first == "//") break;
                                    setter.second.emplace_back(std::string(res->first.begin(), res->first.end()));
                                    piece = res->second;
                              }
                        }
                  }
            }

            std::string analyze_error_msg{};
            if (!AnalyzeSetter(setter, analyze_error_msg)) {
                  error_message = analyze_error_msg;
                  return false;
            }

            auto outputS_str = std::format("set {} = ", setter.first);
            for (const auto &v: setter.second) {
                  outputS_str += v;
                  outputS_str += " ";
            }

      }
      return true;
}


bool Program::ParseVS() {
      MessageManager::Log(MessageType::Normal, "Program::ParseVS - Parsing Vertex Shader");
      spirv_cross::CompilerHLSL comp(_spvVS);
      spirv_cross::ShaderResources resources = comp.get_shader_resources();

      //input stage
      for (auto &resource: resources.stage_inputs) {
            const auto &name = comp.get_name(resource.id);
            const auto &type_id = comp.get_name(resource.base_type_id);
            auto str1 = std::format("\tinput_stage : name {}, type {}.", name, type_id);
            std::printf("%s\n", str1.c_str());
      }

      for (auto &resource: resources.storage_buffers) {
            auto &type = comp.get_type(resource.base_type_id);

            uint32_t set = comp.get_decoration(resource.id, spv::DecorationDescriptorSet);
            uint32_t binding = comp.get_decoration(resource.id, spv::Decoration::DecorationBinding);
            std::string resoure_name = comp.get_name(resource.id);
            std::string type_name = comp.get_name(resource.base_type_id);

            auto str1 = std::format("storage_buffers : Set: {}, Binding {}, resourece name {}, warpper type name: {}.",
                                    set, binding, resoure_name, type_name);
            std::printf("%s\n", str1.c_str());

            uint32_t contained_type_id = type.member_types[0];
            auto contained_type = comp.get_type(contained_type_id);
            std::string contained_type_name = comp.get_name(contained_type.self);

            uint32_t member_count = contained_type.member_types.size();

            auto strSubType = std::format("\t\tcontained type name {}, member count {}.", contained_type_name,
                                          member_count);
            std::printf("%s\n", strSubType.c_str());

            for (uint32_t i = 0; i < member_count; i++) {
                  auto &member_type = comp.get_type(contained_type.member_types[i]);
                  const std::string &member_type_name = comp.get_name(member_type.self);

                  size_t member_size = comp.get_declared_struct_member_size(contained_type, i);
                  size_t offset = comp.type_struct_member_offset(contained_type, i);

                  const std::string &member_name = comp.get_member_name(contained_type.self, i);
                  auto str = std::format("\t\t{}, offset {}, size {}", member_name, offset, member_size);
                  std::printf("%s\n", str.c_str());
            }
      }

      for (auto &resource: resources.separate_images) {
            auto &type = comp.get_type(resource.base_type_id);
            uint32_t member_count = type.member_types.size();

            uint32_t set = comp.get_decoration(resource.id, spv::DecorationDescriptorSet);
            uint32_t binding = comp.get_decoration(resource.id, spv::Decoration::DecorationBinding);

            const std::string& res_name = comp.get_name(resource.id);
            auto str1 = std::format("separate_images : Set: {}, Binding {}, name {}.", set, binding, res_name);
            std::printf("%s\n", str1.c_str());
      }

      for(auto &resource : resources.push_constant_buffers) {
            auto& type = comp.get_type(resource.base_type_id);
            uint32_t member_count = type.member_types.size();
            const std::string& res_name = comp.get_name(resource.id);

            for(int i = 0; i< member_count; i++){
                  auto& member_type = comp.get_type(type.member_types[i]);
                  const std::string& member_type_name = comp.get_name(member_type.self);

                  size_t member_size = comp.get_declared_struct_member_size(type, i);
                  size_t offset = comp.type_struct_member_offset(type, i);

                  const std::string& member_name = comp.get_member_name(type.self, i);
                  auto str = std::format("push_constant_buffers {}, offset {}, size {}", member_name, offset, member_size);
                  std::printf("%s\n", str.c_str());

                  if(i == member_count - 1){
                        _pushConstantRange_temp.offset = 0;
                        _pushConstantRange_temp.size = offset + member_size;
                  }
            }
      }

      return true;
}

bool Program::ParsePS() {
      MessageManager::Log(MessageType::Normal, "Program::ParsePS - Parsing Pixel Shader");
      spirv_cross::CompilerHLSL comp(_spvPS);
      spirv_cross::ShaderResources resources = comp.get_shader_resources();

      auto output_target_count = resources.stage_outputs.size();

      for (auto &resource: resources.stage_outputs) {
            const auto &name = comp.get_name(resource.id);
            const auto &type_id = comp.get_name(resource.base_type_id);

            auto str1 = std::format("\t stage_outputs : name {}, type {}.", name, type_id);
            std::printf("%s\n", str1.c_str());
      }

      if(_colorBlendAttachmentState_temp.size() != output_target_count){
            auto str = std::format("Program::ParsePS - Output target count mismatch, expected {}, got {}, please add or move #set color_blend in shader source code.", output_target_count, _colorBlendAttachmentState_temp.size());
            MessageManager::Log(MessageType::Warning, str);
            return false;
      }



      for (auto &resource: resources.storage_buffers) {
            auto &type = comp.get_type(resource.base_type_id);

            uint32_t set = comp.get_decoration(resource.id, spv::DecorationDescriptorSet);
            uint32_t binding = comp.get_decoration(resource.id, spv::Decoration::DecorationBinding);
            std::string resoure_name = comp.get_name(resource.id);
            std::string type_name = comp.get_name(resource.base_type_id);

            auto str1 = std::format("storage_buffers : Set: {}, Binding {}, resourece name {}, warpper type name: {}.",
                                    set, binding, resoure_name, type_name);
            std::printf("%s\n", str1.c_str());

            uint32_t contained_type_id = type.member_types[0];
            auto contained_type = comp.get_type(contained_type_id);
            std::string contained_type_name = comp.get_name(contained_type.self);

            uint32_t member_count = contained_type.member_types.size();

            auto strSubType = std::format("\t\tcontained type name {}, member count {}.", contained_type_name,
                                          member_count);
            std::printf("%s\n", strSubType.c_str());

            for (uint32_t i = 0; i < member_count; i++) {
                  auto &member_type = comp.get_type(contained_type.member_types[i]);
                  const std::string &member_type_name = comp.get_name(member_type.self);

                  size_t member_size = comp.get_declared_struct_member_size(contained_type, i);
                  size_t offset = comp.type_struct_member_offset(contained_type, i);

                  const std::string &member_name = comp.get_member_name(contained_type.self, i);
                  auto str = std::format("\t\t{}, offset {}, size {}", member_name, offset, member_size);
                  std::printf("%s\n", str.c_str());
            }
      }

      for (auto &resource: resources.separate_images) {
            auto &type = comp.get_type(resource.base_type_id);
            uint32_t member_count = type.member_types.size();

            uint32_t set = comp.get_decoration(resource.id, spv::DecorationDescriptorSet);
            uint32_t binding = comp.get_decoration(resource.id, spv::Decoration::DecorationBinding);

            std::string res_name = comp.get_name(resource.id);
            auto str1 = std::format("separate_images : Set: {}, Binding {}, name {}.", set, binding, res_name);
            std::printf("%s\n", str1.c_str());
      }

      return true;
}

bool Program::AnalyzeSetter(const std::pair<std::string, std::vector<std::string> > &setter, std::string &error_msg) {
      static std::unordered_map<std::string, std::unordered_map<std::string, uint64_t> > setter_map{
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

      auto CheckAndFetchValue = [&](const std::string &the_key, const std::string &the_value,
                                    std::string &error) -> std::optional<uint64_t> {
            if (const auto it = setter_map.find(the_key); it != setter_map.end()) {
                  if (const auto it2 = it->second.find(the_value); it2 != it->second.end()) {
                        return it2->second;
                  } else {
                        error = std::format(R"(Invalid value "{}" for key "{}", )",
                                                   the_value, the_key);
                        error += std::format("Valid values are : \n\t\t");
                        for (const auto &[k, v]: it->second) {
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

      auto Analyze = [&]<typename T0>(T0 &input, const std::string &key, const std::string &the_value,
                                      std::string &error) -> bool {
                                            if (const auto res = CheckAndFetchValue(key, the_value, error_msg); res.has_value()) {
                                                  input = *((T0 *) &res.value());
                                                  return true;
                                            }
                                            return false;
                                      };

      auto ErrorArgumentUnmatching = [](const std::string& key, uint32_t argument_target, uint32_t error_argument, std::string& error_msg, const std::string& argument_format) -> bool {
            error_msg =  std::format("Invalid argument count for key \"{}\". Expected {}, got {}, format: [{}].",
                               key, argument_target, error_argument, argument_format);
            return false;
      };

      auto ErrorArgument = [](const std::string& key, uint32_t err_target, const std::string& value, std::string& error_msg, const std::string& vaild_values) -> bool {
            error_msg = std::format("Invalid argument {} for key \"{}\". Expected [{}], got \"{}\".", key, err_target, vaild_values, value);
            return false;
      };

      const auto &key = setter.first;
      const auto &values = setter.second;

      if (key == "topology") {
            if (!Analyze(_inputAssemblyStateCreateInfo_temp.topology, key, values[0], error_msg)) return false;
      } else if (key == "polygon_mode") {
            if (!Analyze(_rasterizationStateCreateInfo_temp.polygonMode, key, values[0], error_msg)) return false;
      } else if (key == "cull_mode") {
            if (!Analyze(_rasterizationStateCreateInfo_temp.cullMode, key, values[0], error_msg)) return false;
      } else if (key == "front_face") {
            if (!Analyze(_rasterizationStateCreateInfo_temp.frontFace, key, values[0], error_msg)) return false;
      }  else if (key == "depth_write") {
            if (!Analyze(_depthStencilStateCreateInfo_temp.depthWriteEnable, key, values[0], error_msg)) return false;
      } else if (key == "depth_test") {
            _depthStencilStateCreateInfo_temp.depthTestEnable = true;
            if (!Analyze(_depthStencilStateCreateInfo_temp.depthCompareOp, key, values[0], error_msg)) return false;
      } else if (key == "vs_location") {
            if(values.size() == 4) { // location, bind, format, offset
                  uint32_t location;
                  uint32_t binding;
                  uint32_t offset;
                  try {
                        location = std::stoi(values[0]);
                  } catch (const auto& what) {
                        error_msg = std::format("Invalid argument 1 for key \"{}\". Expected integer, got \"{}\".", key, values[0]);
                        return false;
                  }

                  try {
                        binding = std::stoi(values[1]);
                  } catch (const auto& what) {
                        error_msg = std::format("Invalid argument 2 for key \"{}\". Expected integer, got \"{}\".", key, values[1]);
                        return false;
                  }

                  try {
                        offset = std::stoi(values[3]);
                  } catch (const auto& what) {
                        error_msg = std::format("Invalid argument 4 for key \"{}\". Expected integer, got \"{}\".", key, values[3]);
                        return false;
                  }

                  VkFormat format = GetVkFormatFromStringSimpled(values[2]);

                  if(format == VK_FORMAT_UNDEFINED) {
                        return ErrorArgument(key, 3, values[2], error_msg, "(format)");
                  }

                  VkVertexInputAttributeDescription att_desc{
                        .location = location,
                        .binding = binding,
                        .format = format,
                        .offset = offset
                  };

                  _vertexInputAttributeDescription_temp.push_back(att_desc);

                  _vertexInputStateCreateInfo_temp.pVertexAttributeDescriptions = _vertexInputAttributeDescription_temp.data();
                  _vertexInputStateCreateInfo_temp.vertexAttributeDescriptionCount = _vertexInputAttributeDescription_temp.size();
            }else {
                  return ErrorArgumentUnmatching(key, 4, values.size(), error_msg, "location, bind, format, offset");
            }
      } else if (key == "vs_binding") {
            if(values.size() == 3) {  // binding, stride_size, binding_rate
                  uint32_t binding;
                  uint32_t stride_size;

                  try {
                        binding = std::stoi(values[0]);
                  } catch (const auto& what) {
                        error_msg = std::format("Invalid argument 1 for key \"{}\". Expected integer, got \"{}\".", key, values[0]);
                        return false;
                  }

                  try {
                        stride_size = std::stoi(values[1]);
                  } catch (const auto& what) {
                        error_msg = std::format("Invalid argument 2 for key \"{}\". Expected integer, got \"{}\".", key, values[1]);
                        return false;
                  }

                  VkVertexInputBindingDescription binding_desc {
                        .binding = binding,
                        .stride = stride_size,
                        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
                  };

                  if(values[2] == "vertex") {
                        binding_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
                  }else if(values[2] == "instance") {
                        binding_desc.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
                  }else {
                        return ErrorArgument(key, 3, values[2], error_msg, "vertex, instance");
                  }
                  _vertexInputBindingDescription_temp.push_back(binding_desc);
                  _vertexInputStateCreateInfo_temp.pVertexBindingDescriptions = _vertexInputBindingDescription_temp.data();
                  _vertexInputStateCreateInfo_temp.vertexBindingDescriptionCount = _vertexInputBindingDescription_temp.size();

            } else {
                  return ErrorArgumentUnmatching(key, 4, values.size(), error_msg, "binding, stride_size, binding_rate");
            }
      } else if (key == "depth_bias") {
            if(values.size() == 3) {  // depthBiasConstantFactor, depthBiasClamp, depthBiasSlopeFactor
                  float depthBiasConstantFactor;
                  float depthBiasClamp;
                  float depthBiasSlopeFactor;

                  try {
                        depthBiasConstantFactor = std::stof(values[0]);
                  } catch (const auto& what) {
                        error_msg = std::format("Invalid argument 1 for key \"{}\". Expected float, got \"{}\".", key, values[0]);
                        return false;
                  }

                  try {
                        depthBiasClamp = std::stof(values[1]);
                  } catch (const auto& what) {
                        error_msg = std::format("Invalid argument 2 for key \"{}\". Expected float, got \"{}\".", key, values[1]);
                        return false;
                  }

                  try {
                        depthBiasSlopeFactor = std::stof(values[2]);
                  } catch (const auto& what) {
                        error_msg = std::format("Invalid argument 3 for key \"{}\". Expected float, got \"{}\".", key, values[2]);
                        return false;
                  }

                  _rasterizationStateCreateInfo_temp.depthBiasEnable = true;
                  _rasterizationStateCreateInfo_temp.depthBiasConstantFactor = depthBiasConstantFactor;
                  _rasterizationStateCreateInfo_temp.depthBiasClamp = depthBiasClamp;
                  _rasterizationStateCreateInfo_temp.depthBiasSlopeFactor = depthBiasSlopeFactor;
            } else {
                  return ErrorArgumentUnmatching(key, 3, values.size(), error_msg, "depth_bias_constant_factor, depth_bias_clamp, depth_bias_slope_factor");
            }
      } else if (key == "depth_bounds_test") {
            if(values.size() == 2) {  // min_depth, max_depth
                  float min_depth;
                  float max_depth;

                  try {
                        min_depth = std::stof(values[0]);
                  } catch (const auto& what) {
                        error_msg = std::format("Invalid argument 1 for key \"{}\". Expected float, got \"{}\".", key, values[0]);
                        return false;
                  }

                  try {
                        max_depth = std::stof(values[1]);
                  } catch (const auto& what) {
                        error_msg = std::format("Invalid argument 2 for key \"{}\". Expected float, got \"{}\".", key, values[1]);
                        return false;
                  }

                  _depthStencilStateCreateInfo_temp.depthBoundsTestEnable = true;
                  _depthStencilStateCreateInfo_temp.minDepthBounds = min_depth;
                  _depthStencilStateCreateInfo_temp.maxDepthBounds = max_depth;
            } else {
                  return ErrorArgumentUnmatching(key, 2, values.size(), error_msg, "min_depth, max_depth");
            }
      } else if (key == "line_width") {
            float line_width;

            try {
                  line_width = std::stof(values[0]);
            } catch (const auto& what) {
                  error_msg = std::format("Invalid argument 1 for key \"{}\". Expected float, got \"{}\".", key, values[0]);
                  return false;
            }
            _rasterizationStateCreateInfo_temp.lineWidth = line_width;

      } else if (key == "color_blend") {
            if(values.size() == 1) {  // blend_enable
                  if(values[0] == "false") {
                        VkPipelineColorBlendAttachmentState default_profile {};
                        default_profile.blendEnable = VK_FALSE;
                        _colorBlendAttachmentState_temp.push_back(default_profile);
                  } else if(values[0] == "add") {
                        VkPipelineColorBlendAttachmentState additive_blend = {};
                        additive_blend.blendEnable = VK_TRUE;
                        additive_blend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
                        additive_blend.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
                        additive_blend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
                        additive_blend.colorBlendOp = VK_BLEND_OP_ADD;
                        additive_blend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                        additive_blend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
                        additive_blend.alphaBlendOp = VK_BLEND_OP_ADD;
                        _colorBlendAttachmentState_temp.push_back(additive_blend);
                  } else if(values[0] == "multiply") {
                        VkPipelineColorBlendAttachmentState multiplicative_blend = {};
                        multiplicative_blend.blendEnable = VK_TRUE;
                        multiplicative_blend.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
                        multiplicative_blend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
                        multiplicative_blend.colorBlendOp = VK_BLEND_OP_MULTIPLY_EXT;
                        multiplicative_blend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                        multiplicative_blend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                        multiplicative_blend.alphaBlendOp = VK_BLEND_OP_MULTIPLY_EXT;
                        multiplicative_blend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
                        _colorBlendAttachmentState_temp.push_back(multiplicative_blend);
                  } else if(values[0] == "sub") {
                        VkPipelineColorBlendAttachmentState subtractive_blend = {};
                        subtractive_blend.blendEnable = VK_TRUE;
                        subtractive_blend.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
                        subtractive_blend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
                        subtractive_blend.colorBlendOp = VK_BLEND_OP_SUBTRACT;
                        subtractive_blend.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
                        subtractive_blend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                        subtractive_blend.alphaBlendOp = VK_BLEND_OP_SUBTRACT;
                        subtractive_blend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
                        _colorBlendAttachmentState_temp.push_back(subtractive_blend);
                  } else if(values[0] == "alpha") {
                        VkPipelineColorBlendAttachmentState alpha_blend = {};
                        alpha_blend.blendEnable = VK_TRUE;
                        alpha_blend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
                        alpha_blend.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
                        alpha_blend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                        alpha_blend.colorBlendOp = VK_BLEND_OP_ADD;
                        alpha_blend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                        alpha_blend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
                        alpha_blend.alphaBlendOp = VK_BLEND_OP_ADD;
                        _colorBlendAttachmentState_temp.push_back(alpha_blend);
                  } else if(values[0] == "screen") {
                        VkPipelineColorBlendAttachmentState screen_blend = {};
                        screen_blend.blendEnable = VK_TRUE;
                        screen_blend.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
                        screen_blend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
                        screen_blend.colorBlendOp = VK_BLEND_OP_SCREEN_EXT;
                        screen_blend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                        screen_blend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                        screen_blend.alphaBlendOp = VK_BLEND_OP_SCREEN_EXT;
                        screen_blend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
                        _colorBlendAttachmentState_temp.push_back(screen_blend);
                  } else if(values[0] == "darken") {
                        VkPipelineColorBlendAttachmentState darken_blend = {};
                        darken_blend.blendEnable = VK_TRUE;
                        darken_blend.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
                        darken_blend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
                        darken_blend.colorBlendOp = VK_BLEND_OP_MIN;
                        darken_blend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                        darken_blend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                        darken_blend.alphaBlendOp = VK_BLEND_OP_MIN;
                        darken_blend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
                        _colorBlendAttachmentState_temp.push_back(darken_blend);
                  } else if(values[0] == "lighten") {
                        VkPipelineColorBlendAttachmentState lighten_blend = {};
                        lighten_blend.blendEnable = VK_TRUE;
                        lighten_blend.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
                        lighten_blend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
                        lighten_blend.colorBlendOp = VK_BLEND_OP_MAX;
                        lighten_blend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                        lighten_blend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                        lighten_blend.alphaBlendOp = VK_BLEND_OP_MAX;
                        lighten_blend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
                        _colorBlendAttachmentState_temp.push_back(lighten_blend);
                  } else if(values[0] == "difference") {
                        VkPipelineColorBlendAttachmentState difference_blend = {};
                        difference_blend.blendEnable = VK_TRUE;
                        difference_blend.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
                        difference_blend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
                        difference_blend.colorBlendOp = VK_BLEND_OP_DIFFERENCE_EXT;
                        difference_blend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                        difference_blend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                        difference_blend.alphaBlendOp = VK_BLEND_OP_DIFFERENCE_EXT;
                        difference_blend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
                        _colorBlendAttachmentState_temp.push_back(difference_blend);
                  } else if(values[0] == "exclusion") {
                        VkPipelineColorBlendAttachmentState exclusion_blend = {};
                        exclusion_blend.blendEnable = VK_TRUE;
                        exclusion_blend.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
                        exclusion_blend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
                        exclusion_blend.colorBlendOp = VK_BLEND_OP_EXCLUSION_EXT;
                        exclusion_blend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                        exclusion_blend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                        exclusion_blend.alphaBlendOp = VK_BLEND_OP_EXCLUSION_EXT;
                        exclusion_blend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
                        _colorBlendAttachmentState_temp.push_back(exclusion_blend);
                  } else {
                        return ErrorArgument(key, 1, values[0], error_msg, "(false) or (add, multiply, sub, alpha, screen, darken, lighten, difference, exclusion)");
                        //return ErrorArgumentUnmatching(key, 1, values.size(), error_msg, "if you enable color blend, please use argument: (defualt) or (colorWriteMask, dstAlphaBlendFactor, srcAlphaBlendFactor, colorBlendOp, alphaBlendOp)");
                  }
                  _colorBlendStateCreateInfo_temp.attachmentCount = _colorBlendAttachmentState_temp.size();
                  _colorBlendStateCreateInfo_temp.pAttachments = _colorBlendAttachmentState_temp.data();
            } else {
                  return ErrorArgumentUnmatching(key, 1, values.size(), error_msg, "(blend_enable) or (colorWriteMask, dstAlphaBlendFactor, srcAlphaBlendFactor, colorBlendOp, alphaBlendOp)");
            }
      } else if (key == "rt") {
            for(int idx = 0; idx < values.size(); idx++) {
                  VkFormat vk_format = GetVkFormatFromStringSimpled(values[idx]);

                  if(vk_format == VK_FORMAT_UNDEFINED) {
                        error_msg = std::format("Invalid argument {} for key \"{}\". Expected a vaild format, got \"{}\".", idx, key, values[idx]);
                        return false;
                  }

                  _renderTargetFormat_temp.push_back(vk_format);
            }
            _renderingCreateInfo_temp.colorAttachmentCount = _renderTargetFormat_temp.size();
            _renderingCreateInfo_temp.pColorAttachmentFormats = _renderTargetFormat_temp.data();
      } else if (key == "ds") {
            if(values.size() == 1) {
                  VkFormat vk_format = GetVkFormatFromStringSimpled(values[0]);
                  if(!IsDepthStencilOnlyFormat(vk_format)) {
                        error_msg = std::format("Invalid argument {} for key \"{}\". Expected a vaild format, got \"{}\", vaild format:[d16_unorm_s8_uint, d24_unorm_s8_uint, d32_sfloat_s8_uint].", 0, key, values[0]);
                        return false;
                  }

                  _renderingCreateInfo_temp.depthAttachmentFormat = vk_format;
                  _renderingCreateInfo_temp.stencilAttachmentFormat = vk_format;

            } else if(values.size() == 2) {
                  VkFormat depth_format = GetVkFormatFromStringSimpled(values[0]);
                  VkFormat stencil_format = GetVkFormatFromStringSimpled(values[1]);

                  if(!IsDepthOnlyFormat(depth_format)) {
                        error_msg = std::format("Invalid argument 1 for key \"{}\". Expected a vaild format, got \"{}\", vaild format:[d16_unorm, d32_sfloat].", key, values[0]);
                        return false;
                  }

                  if(!IsStencilOnlyFormat(stencil_format)) {
                        error_msg = std::format("Invalid argument 2 for key \"{}\". Expected a vaild format, got \"{}\", vaild format:[s8_uint].", key, values[1]);
                        return false;
                  }

                  _renderingCreateInfo_temp.depthAttachmentFormat = depth_format;
                  _renderingCreateInfo_temp.stencilAttachmentFormat = stencil_format;
            } else {
                  error_msg = std::format("Invalid argument count for key \"{}\". Expected 1 or 2, got {}, format: (using depth_stencil (depth_format) or depth, stencil (depth_format, stencil_format)).", key, values.size());
                  return false;
            }
      } else {
            error_msg = std::format("Invalid setter key \"{}\" .", key);
            return false;
      }

      return true;
}

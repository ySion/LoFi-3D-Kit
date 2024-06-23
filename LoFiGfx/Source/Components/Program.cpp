#define NOMINMAX
#include "Program.h"

#include "../Context.h"
#include "../Third/dxc/dxcapi.h"
#include <wrl/client.h>

#include "../Message.h"
#include "SDL3/SDL.h"
#include "../Third/spirv-cross/spirv_cross.hpp"
#include "../Third/spirv-cross/spirv_hlsl.hpp"

using namespace LoFi::Component;

template<class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

Program::Program(entt::entity id) : _id(id) {}

bool Program::CompileFromSourceCode(std::string_view name, std::string_view source) {
      _codes = source;
      _programName = name;

      std::vector<uint32_t> spv[5]{};

      std::vector<std::string> success_shaders{};
      if (_codes.find("VSMain") != std::string::npos) {
            if (!CompileFromCode(L"VSMain", L"vs_6_6", spv[0])) {
                  return false;
            }
            success_shaders.emplace_back("Vertex");
      }

      if (_codes.find("PSMain") != std::string::npos) {
            if (!CompileFromCode(L"PSMain", L"ps_6_6", spv[1])) {
                  return false;
            }
            success_shaders.emplace_back("Pixel");
      }

      if (_codes.find("CSMain") != std::string::npos) {
            if (!CompileFromCode(L"CSMain", L"cs_6_6", spv[2])) {
                  return false;
            }
            success_shaders.emplace_back("Compute");
      }

      if (_codes.find("MSMain") != std::string::npos) {
            if (!CompileFromCode(L"MSMain", L"ms_6_6", spv[3])) {
                  return false;
            }
            success_shaders.emplace_back("Mesh");
      }

      if (_codes.find("ASMain") != std::string::npos) {
            if (!CompileFromCode(L"ASMain", L"as_6_6", spv[4])) {
                  return false;
            }
            success_shaders.emplace_back("Task");
      }
      auto str = std::format("Program::CompileFromSourceCode - Compiled Shader Program \"{}\"  -- Success\n", _programName);

      for (const auto& success_shader : success_shaders) {
            str += std::format("\t\t{} Shader -- SUCCESS \n", success_shader);
      }
      //start parse

      MessageManager::Log(MessageType::Normal, str);

      _spvVS = std::move(spv[0]);
      _spvPS = std::move(spv[1]);
      _spvCS = std::move(spv[2]);
      _spvMS = std::move(spv[3]);
      _spvAS = std::move(spv[4]);

      for (const auto& success_shader : success_shaders) {
            if (success_shader == "Pixel") {
                  ParsePS();
            }
      }

      return true;
}

bool Program::CompileFromCode(const wchar_t* entry_point, const wchar_t* shader_type, std::vector<uint32_t>& spv) {

      auto& compiler = *(LoFi::Context::Get()->_dxcCompiler);

      DxcBuffer buffer = {
            .Ptr = _codes.data(),
            .Size = (uint32_t)_codes.size(),
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
      compiler.Compile(&buffer, args.data(), (uint32_t)(args.size()), nullptr, IID_PPV_ARGS(&dxc_result));

      HRESULT result_code = E_FAIL;
      dxc_result->GetStatus(&result_code);
      ComPtr<IDxcBlobEncoding> dxc_error = nullptr;
      dxc_result->GetErrorBuffer(&dxc_error);


      if (FAILED(result_code)) { // compile error
            bool const has_errors = (dxc_error != nullptr && dxc_error->GetBufferPointer() != nullptr);
            auto error_msg = std::format("Program::CompileFromCode - Failed to compile shader \"{}\" for entry point \"{}\":\n{}", _programName, (const char*)entry_point, has_errors ? (char const*)dxc_error->GetBufferPointer() : "");
            MessageManager::Log(MessageType::Warning, error_msg);
            return false;
      }

      if (dxc_error != nullptr) { // has_warnings
            if (dxc_error->GetBufferPointer() != nullptr) {
                  auto warning_msg = std::format("Program::CompileFromCode - Compiled shader \"{}\" for entry point \"{}\" with Warning(s):\n{}", _programName, (const char*)entry_point, (char const*)dxc_error->GetBufferPointer());
                  MessageManager::Log(MessageType::Warning, warning_msg);

            }
      }


      ComPtr<IDxcBlob> spv_object{};
      dxc_result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&spv_object), nullptr);

      if(spv_object->GetBufferSize() % sizeof(uint32_t) != 0) {
            auto str = std::format(R"(Program::CompileFromCode - Invalid SPIR-V size shader compile "{}" for entry point "{}". Invalid size: "{}")", _programName, (const char*)entry_point, spv_object->GetBufferSize());
            MessageManager::Log(MessageType::Warning, str);
            return false;
      }

      spv.resize(spv_object->GetBufferSize() / sizeof(uint32_t));
      memcpy(spv.data(), spv_object->GetBufferPointer(), spv_object->GetBufferSize());

      return true;
}

bool Program::ParsePS()
{
      MessageManager::Log(MessageType::Normal, "Program::ParsePS - Parsing Pixel Shader");
      spirv_cross::CompilerHLSL comp(_spvPS);
      spirv_cross::ShaderResources resources = comp.get_shader_resources();

      //Push Constants

      for (auto& resource : resources.storage_buffers) {

            auto &type = comp.get_type(resource.base_type_id);



            uint32_t set = comp.get_decoration(resource.id, spv::DecorationDescriptorSet);
            uint32_t binding = comp.get_decoration(resource.id, spv::Decoration::DecorationBinding);
            std::string resoure_name = comp.get_name(resource.id);
            std::string type_name = comp.get_name(resource.base_type_id);

            auto str1 = std::format("storage_buffers : Set: {}, Binding {}, resourece name {}, warpper type name: {}.", set, binding, resoure_name, type_name);
            std::printf("%s\n", str1.c_str());

            uint32_t contained_type_id = type.member_types[0];
            auto contained_type = comp.get_type(contained_type_id);
            std::string contained_type_name = comp.get_name(contained_type.self);

            uint32_t member_count = contained_type.member_types.size();

            auto strSubType = std::format("\t\tcontained type name {}, member count {}.", contained_type_name, member_count);
            std::printf("%s\n", strSubType.c_str());

            for (uint32_t i = 0; i < member_count; i++) {
                  auto &member_type = comp.get_type(contained_type.member_types[i]);
                  const std::string& member_type_name = comp.get_name(member_type.self);

                  size_t member_size = comp.get_declared_struct_member_size(contained_type, i);
                  size_t offset = comp.type_struct_member_offset(contained_type, i);

                  const std::string& member_name = comp.get_member_name(contained_type.self, i);
                  auto str = std::format("\t\t{}, offset {}, size {}", member_name, offset, member_size);
                  std::printf("%s\n", str.c_str());
            }
      }

      for (auto& resource : resources.separate_images) {
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


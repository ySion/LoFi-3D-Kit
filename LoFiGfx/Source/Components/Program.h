#pragma once

#include "../Helper.h"

namespace LoFi::Component {

      class Program {
      public:

            NO_COPY_MOVE_CONS(Program);

            ~Program() = default;

            explicit Program(entt::entity id);

            bool CompileFromSourceCode(std::string_view name, std::string_view source);

      private:

            bool CompileFromCode(const wchar_t* entry_point, const wchar_t* shader_type, std::vector<uint32_t>& spv);

            bool ParsePS();

      private:

            entt::entity _id {};

            std::string _programName {};

            std::string _codes {};

            std::vector<uint32_t> _spvVS {};

            std::vector<uint32_t> _spvPS {};

            std::vector<uint32_t> _spvCS {};

            std::vector<uint32_t> _spvMS {};

            std::vector<uint32_t> _spvAS {};
      };
}
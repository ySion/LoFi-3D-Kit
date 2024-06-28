//
// Created by starr on 2024/6/23.
//

#pragma once

#include "../Helper.h"

namespace LoFi {
      class Context;
}

namespace LoFi::Component {

      struct GraphicKernelStructMemberInfo {
            uint32_t StructIndex;
            uint32_t Size;
            uint32_t Offset;
      };

      struct GraphicKernelStructInfo {
            uint32_t Index;
            uint32_t Size;
      };

      class GraphicKernel {
      public:
            NO_COPY_MOVE_CONS(GraphicKernel);

            explicit GraphicKernel(entt::entity id);

            ~GraphicKernel();

            [[nodiscard]] VkPipeline GetPipeline() const { return _pipeline; }

            [[nodiscard]] VkPipelineLayout GetPipelineLayout() const { return _pipelineLayout; }

            [[nodiscard]] VkPipelineLayout* GetPipelineLayoutPtr() { return &_pipelineLayout; }

            [[nodiscard]] const entt::dense_map<std::string, GraphicKernelStructInfo>& GetStructTable() const {return _structTable;}

            [[nodiscard]] const entt::dense_map<std::string, uint32_t>& GetSampledTextureTable() const {return _sampledTextureTable;}

            [[nodiscard]] const entt::dense_map<std::string, GraphicKernelStructMemberInfo>& GetStructMemberTable() const {return _structMemberTable;}

            [[nodiscard]] const VkPushConstantRange& GetBindlessInfoPushConstantRange() const {return _pushConstantRange;}

      private:

            bool CreateFromProgram(entt::entity program);

            friend class ::LoFi::Context;

      private:
            entt::entity _id = entt::null;

            VkPipeline _pipeline{};

            VkPipelineLayout _pipelineLayout{};

            entt::dense_map<std::string, GraphicKernelStructInfo> _structTable{};

            entt::dense_map<std::string, uint32_t> _sampledTextureTable{};

            entt::dense_map<std::string, GraphicKernelStructMemberInfo> _structMemberTable{};

            VkPushConstantRange _pushConstantRange{};

      };
}

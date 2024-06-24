//
// Created by starr on 2024/6/23.
//

#pragma once

#include "../Helper.h"

namespace LoFi::Component {

      struct GraphicKernelConfig {
            VkFormat RenderTargetFormat[8] = {
                  VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED,
                  VK_FORMAT_UNDEFINED,VK_FORMAT_UNDEFINED,VK_FORMAT_UNDEFINED,VK_FORMAT_UNDEFINED};

            VkFormat DepthFormat = VK_FORMAT_D32_SFLOAT_S8_UINT;
            VkFormat StencilFormat = VK_FORMAT_D32_SFLOAT_S8_UINT;

            VkVertexInputAttributeDescription VertexInputAttributeDescription[8] = {
                  VkVertexInputAttributeDescription{0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 0},
                  VkVertexInputAttributeDescription{1, 0, VK_FORMAT_R32G32B32_SFLOAT, 16},
            };
            VkVertexInputBindingDescription VertexInputBindDescription[8] = {
                  VkVertexInputBindingDescription{0, 32, VK_VERTEX_INPUT_RATE_VERTEX},
            };

            VkPrimitiveTopology PrimitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            VkPolygonMode PolygonMode = VK_POLYGON_MODE_FILL;
            VkCullModeFlags CullMode = VK_CULL_MODE_BACK_BIT;
            bool DepthTestEnable = false;
            bool DepthWriteEnable = false;
            VkCompareOp DepthCompareOp = VK_COMPARE_OP_LESS;
            VkStencilOpState StencilOpFront {};
            VkStencilOpState StencilOpBack {};
      };

      class GraphicKernel {
      public:

            NO_COPY_MOVE_CONS(GraphicKernel);

            explicit  GraphicKernel(entt::entity id);

            ~GraphicKernel();

            bool CreateFromProgram(entt::entity program, const GraphicKernelConfig& config = {}) ;

      private:

            entt::entity _id = entt::null;

            VkPipeline _pipeline {};

            VkPipelineLayout _pipelineLayout {};
      };
}
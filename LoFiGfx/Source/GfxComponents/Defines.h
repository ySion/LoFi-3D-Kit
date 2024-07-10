//
// Created by Arzuo on 2024/7/4.
//

#pragma once

#include "../Helper.h"

namespace LoFi::Component::Gfx {

      struct TagMultiFrameResourceOrBufferChanged {};

      struct TagMultiFrameResourceOrBufferUpdateCompleted {};

      struct KernelParamResource {
            uint32_t Modified = 0;
            std::vector<uint8_t> CachedBufferData{};
            std::array<entt::entity, 3> Buffers{};
      };

      struct PushConstantMemberInfo {
            uint32_t Offset;
            uint32_t Size;
      };

      enum class ProgramType {
            UNKNOWN,
            GRAPHICS,
            COMPUTE,
            MESH,
            RAY_TRACING
      };

      enum class ShaderResource : uint32_t {
            READ_BUFFER = 0,
            WRITE_BUFFER = 1,
            READ_WRITE_BUFFER = 2,
            READ_TEXTURE = 3,
            WRITE_TEXTURE = 4,
            READ_WRITE_TEXTURE = 5,
            SAMPLED_TEXTURE = 6
      };

      struct ShaderResourceInfo {
            ShaderResource Type;
            uint32_t Size;
            uint32_t Offset;
            uint32_t Index;
      };
};
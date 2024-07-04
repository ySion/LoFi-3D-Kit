//
// Created by Arzuo on 2024/7/4.
//

#pragma once

#include "../Helper.h"

namespace LoFi::Component {

      struct TagKernelInstanceParamChanged {};

      struct TagKernelInstanceParamUpdateCompleted {};

      struct KernelParamResource {
            uint32_t Modified = 0;
            std::vector<uint8_t> CachedBufferData{};
            std::array<entt::entity, 3> Buffers{};
      };
};
#pragma once

#include "Defines.h"

namespace LoFi::Component {
      class KernelInstance {
      public:
            NO_COPY_MOVE_CONS(KernelInstance);

            ~KernelInstance();

            // high_dynamic allows binding in a fast frequency, but the speed of gpu access will be more slow
            // if don't used high dynamic, you shouldn't bind resource in a fast frequency like rendering cycle
            explicit KernelInstance(entt::entity id, entt::entity kernel, bool high_dynamic = false);

            [[nodiscard]] entt::entity GetHandle() const { return _id; }

            [[nodiscard]] entt::entity GetKernel() const { return _kernel; }

            [[nodiscard]] bool IsEmptyInstance() const { return _isEmptyInstance; }

            bool BindResource(const std::string& resource_name, entt::entity resource);

            void PushParameterTable(VkCommandBuffer cmd) const;

            static void UpdateInstancesParameterTable();

      public:
            void UpdateParameterTable();

            entt::entity _id;

            entt::entity _kernel;

            KernelParamResource _parameterTableBuffer{};

            uint64_t _parameterTableBufferAddress[3]{};

            bool _isEmptyInstance = false;
      };
}

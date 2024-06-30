//
// Created by Arzuo on 2024/6/28.
//

#pragma once

#include "../Helper.h"
#include "GraphicKernel.h"

namespace LoFi::Component {

      struct TagGrapicsKernelInstanceParameterChanged {};
      struct TagGrapicsKernelInstanceParameterUpdateCompleted {};

      struct FrameResourceBuffer {
            uint32_t Modified = 0;
            std::vector<uint8_t> CachedBufferData{};
            std::array<entt::entity, 3> Buffers{};
      };

      // notice: FrameResource will upload CachedBufferData to gpu buffer when CmdBindGraphicKernelWithFrameResourceToRenderPass() called if IsModified is true
      class GrapicsKernelInstance {
      public:
            NO_COPY_MOVE_CONS(GrapicsKernelInstance);

            ~GrapicsKernelInstance();

            explicit GrapicsKernelInstance(entt::entity id, entt::entity graphics_kernel, bool is_cpu_side = true);

            [[nodiscard]] entt::entity GetHandle() const { return _id; }

            [[nodiscard]] bool IsCpuSide() const { return _isCpuSide; }

            [[nodiscard]] entt::entity GetParentGraphicsKernel() const { return _parent; }

            bool SetParameterStruct(const std::string& struct_name, const void* data); // likes "Info"

            bool SetParameterStructMember(const std::string& struct_member_name, const void* data); // likes "Info.time"

            bool SetParameterTexture(const std::string& texture_name, entt::entity texture);

      private:
            friend class Context;

            void PushResourceChanged();

            void PushBindlessInfo(VkCommandBuffer buf) const;

            entt::entity _id;

            entt::entity _parent; // Graphics kernel or FrameResource

            bool _isCpuSide;

            std::vector<FrameResourceBuffer> _buffers{};

            std::vector<uint32_t> _pushConstantBindlessIndexInfoBuffer{}; // BindlessInfo
      };
}

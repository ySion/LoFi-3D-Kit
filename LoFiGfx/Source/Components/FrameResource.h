//
// Created by Arzuo on 2024/6/28.
//

#pragma once

#include "../Helper.h"
#include "GraphicKernel.h"

namespace LoFi::Component {

      struct TagFrameResourceChanged {};

      struct FrameResourceBuffer {
            bool IsModified = false;
            std::vector<uint8_t> CachedBufferData{};
            std::array <entt::entity, 3> Buffers{};
      };


      // notice: FrameResource will upload CachedBufferData to gpu buffer when CmdBindGraphicKernelWithFrameResourceToRenderPass() called if IsModified is true
      class FrameResource {
      public:
            NO_COPY_MOVE_CONS(FrameResource);

            ~FrameResource();

            explicit FrameResource(entt::entity id, entt::entity graphics_kernel, bool is_cpu_side = true);

            [[nodiscard]] entt::entity GetHandle() const { return _id; }

            [[nodiscard]] bool IsCpuSide() const { return _isCpuSide; }

            [[nodiscard]] entt::entity GetParentGraphicsKernel() const { return _parent; }

            bool SetStruct(const std::string& struct_name, const void* data); // likes "Info"

            bool SetStructMember(const std::string& struct_member_name, const void* data); // likes "Info.time"

            bool SetSampledImage(const std::string& image_name, entt::entity texture);

      private:
            friend class Context;

            void PushResourceChanged();

            void PushBindlessInfo(VkCommandBuffer buf) const;

            entt::entity _id;

            entt::entity _parent; // Graphics kernel or FrameResource

            bool _isCpuSide;

            std::vector<FrameResourceBuffer> _buffers{};

            std::array<std::vector<uint32_t>, 3> _pushConstantBindlessIndexInfoBuffer{}; // BindlessInfo
      };
}

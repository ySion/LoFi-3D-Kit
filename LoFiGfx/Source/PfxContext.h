#pragma once

#include "GfxContext.h"
#include "PtxComponents/RenderNode.h"

namespace LoFi {

      struct FontUV {
            uint32_t x, y, w, h;
      };
      //painter library
      class PfxContext {
            static inline PfxContext* _instance;
      public:
            static PfxContext* Get() { return _instance; }

            NO_COPY_MOVE_CONS(PfxContext);

            ~PfxContext();

            explicit PfxContext();

            bool LoadFont(const char* path);

            [[nodiscard]] entt::entity GetAtlas() const { return _fontAtlas;}

            //entt::entity GenText(wchar_t ch, uint32_t font_size_pixel);

            entt::entity CreateRenderNode();

            void DestroyHandle(entt::entity handle);

            void CmdReset(entt::entity render_node);

            void CmdGenerate(entt::entity render_node);

            void CmdEmitCommand(entt::entity render_node, const std::optional<VkViewport>& viewport = std::nullopt) const;

            void CmdSetVirtualCanvasSize(entt::entity render_node,uint32_t w, uint32_t h);

            void CmdPushScissor(entt::entity render_node, int x, int y, uint32_t width, uint32_t height);

            void CmdPopScissor(entt::entity render_node);

            void CmdDrawRect(entt::entity render_node,uint32_t x, uint32_t y, uint32_t width, uint32_t height, float r, float g, float b, float a);

            //Start a new draw call
            void CmdNewLayer(entt::entity render_node);

      private:
            GfxContext* _gfx;

            entt::entity _fontAtlas;

            VkExtent2D _fontAtlasSize;

            entt::dense_map<wchar_t, FontUV> _fontUVs{};

            entt::registry _world;

            entt::entity _programNormal;
            entt::entity _kernelNormal;
            entt::entity _kernelNormalInstance;

            friend class Component::Ptx::RenderNode;
      };

};


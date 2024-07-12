//
// Created by Arzuo on 2024/7/9.
//

#include "PfxContext.h"
#include "GfxContext.h"
#include "Message.h"
#include "../Third/msdf-atlas-gen/msdf-atlas-gen.h"

using namespace LoFi;
using namespace msdf_atlas;

PfxContext::~PfxContext() {
}

PfxContext::PfxContext() {
      if (GfxContext::Get() == nullptr) {
            const auto err = "GfxContext Should be Init first before PtxContext";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }
      _gfx = GfxContext::Get();
      if (_instance != nullptr) {
            const auto err = "PfxContext Should be Init only once";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }
      _instance = this;

      constexpr auto font_vs = R"(
            //#set polygon_mode = line
            #set cull_mode = none

            layout(location = 0) in ivec3 in_pos_uv_color;

            layout(location = 0) out vec4 out_color;

            void UnpackPos(uint v, out vec2 pos) {
                    pos = vec2(
                            (v & 0xFFFF),
                            ((v >> 16) & 0xFFFF)
                    ) / 65535.0f * 2.0f - vec2(1.0f, 1.0f);
            }

            void UnpackUV(uint v, out vec2 uv) {
                  uv = vec2(
                        (v & 0xFFFF) / 65535.0f,
                        ((v >> 16) & 0xFFFF) / 65535.0f
                  );
            }

            void UnpackColor(uint v, out vec4 color) {
                  color = vec4(
                        (v & 0xFF) / 255.0f,
                        ((v >> 8) & 0xFF) / 255.0f,
                        ((v >> 16) & 0xFF) / 255.0f,
                        ((v >> 24) & 0xFF) / 255.0f
                  );
            }

            void VSMain() {

                  vec2 pos;
                  vec2 uv;
                  vec4 color;

                  UnpackPos(in_pos_uv_color.x, pos);
                  UnpackUV(in_pos_uv_color.y, uv);
                  UnpackColor(in_pos_uv_color.z, color);

                  gl_Position = vec4(pos, 0.0f, 1.0f);
                  out_color = color;
            }
      )";

      constexpr auto font_ps = R"(
            #set rt = r8g8b8a8_unorm
            #set color_blend = add

            layout(location = 0) in vec4 color;

            layout(location = 0) out vec4 outColor;

            void FSMain() {
                  outColor = color; //vec4(color.r, color.r=g=, 1.0, 1.0);
            }
      )";

      _programNormal = _gfx->CreateProgram({font_vs, font_ps});
      _kernelNormal = _gfx->CreateKernel(_programNormal);
      _kernelNormalInstance = _gfx->CreateKernelInstance(_kernelNormal);
}

bool PfxContext::LoadFont(const char* path) {
      if (msdfgen::FreetypeHandle* ft = msdfgen::initializeFreetype()) {
            if (msdfgen::FontHandle* font = msdfgen::loadFont(ft, path)) {
                  std::vector<GlyphGeometry> glyphs{};
                  FontGeometry fontGeometry(&glyphs);
                  Charset charset{};

                  if (!charset.load("f.txt")) {
                        MessageManager::Log(MessageType::Error, "load charset failed");
                        return false;
                  }

                  // for(wchar_t i : charset) {
                  //     wprintf(L"%wc - %X\n", i, i);
                  // }

                  fontGeometry.loadCharset(font, 1.0, charset);

                  const double maxCornerAngle = 3.0;
                  for (GlyphGeometry& glyph : glyphs)
                        glyph.edgeColoring(&msdfgen::edgeColoringInkTrap, maxCornerAngle, 0);

                  GridAtlasPacker packer{};
                  packer.setCellDimensions(32, 32);
                  packer.setDimensions(3200, 3200);
                  packer.setMinimumScale(16.0);
                  packer.setPixelRange(2.0);
                  packer.setMiterLimit(1.0);
                  packer.pack(glyphs.data(), glyphs.size());

                  int width = 0, height = 0;
                  packer.getDimensions(width, height);

                  ImmediateAtlasGenerator<
                        float,
                        4,
                        mtsdfGenerator,
                        BitmapAtlasStorage<byte, 4>
                  > generator(width, height);

                  GeneratorAttributes attributes;
                  generator.setAttributes(attributes);
                  generator.setThreadCount(32);

                  generator.generate(glyphs.data(), glyphs.size());

                  for (const auto& glyph : glyphs) {
                        auto const rect = glyph.getBoxRect();
                        //printf("0x%X - x %d y %d w %d h %d\n", glyph.getCodepoint(), rect.x, rect.y, rect.w, rect.h);
                        _fontUVs[glyph.getCodepoint()] = {(uint32_t)rect.x, (uint32_t)rect.y, (uint32_t)rect.w, (uint32_t)rect.h};
                  }
                  const msdfgen::BitmapConstRef<byte, 4> bitmap = (msdfgen::BitmapConstRef<byte, 4>)generator.atlasStorage();

                  printf("%u x %u\n", bitmap.width, bitmap.height);
                  stbi_write_png("D://test2.png", bitmap.width, bitmap.height, 4, bitmap.pixels, 0);

                  _fontAtlasSize = VkExtent2D{(uint32_t)bitmap.width, (uint32_t)bitmap.height};
                  _fontAtlas = _gfx->CreateTexture2D(VK_FORMAT_R8G8B8A8_UNORM, bitmap.width, bitmap.height);
                  _gfx->FillTexture2D(_fontAtlas, bitmap.pixels, bitmap.width * bitmap.height * 4);

                  msdfgen::destroyFont(font);
            }
            msdfgen::deinitializeFreetype(ft);
      }

      return true;
}

// entt::entity PfxContext::GenText(wchar_t ch, uint32_t font_size_pixel) {
//       //gen squares
//
//       if (!_fontUVs.contains(ch)) {
//             printf("charnot found\n");
//             return entt::null;
//       }
//
//       auto uv = _fontUVs[ch];
//
//       //   std::array square_vt2 = {
//       //       -1.0f, 1.0f , 0.0f, 1.0f,
//       //       1.0f, 1.0f, 1.0f, 1.0f,
//       //       1.0f, -1.0f, 1.0f, 0.0f,
//       //       -1.0f, -1.0f, 0.0f, 0.0f,
//       // };
//
//       std::array square_vt = {
//             -1.0f, 1.0f, (float)uv.x / (float)_fontAtlasSize.width, (float)(uv.y + uv.h) / (float)_fontAtlasSize.height, // top left
//             1.0f, 1.0f, (float)(uv.x + uv.w) / (float)_fontAtlasSize.width, (float)(uv.y + uv.h) / (float)_fontAtlasSize.height, // top right
//             1.0f, -1.0f, (float)(uv.x + uv.w) / (float)_fontAtlasSize.width, (float)uv.y / (float)_fontAtlasSize.height, // botom left
//             -1.0f, -1.0f, (float)uv.x / (float)_fontAtlasSize.width, (float)uv.y / (float)_fontAtlasSize.height // bottom right
//       };
//
//       const auto buffer = _gfx->CreateBuffer(square_vt);
//       printf("yes\n");
//       return buffer;
// }

entt::entity PfxContext::CreateRenderNode() {
      const entt::entity id = _world.create();

      try {
            _world.emplace<LoFi::Component::Ptx::RenderNode>(id, id);
      } catch (const std::exception& w) {
            _world.destroy(id);
            const auto err = std::format("PfxContext::CreateRenderNode - {}", w.what());
            MessageManager::Log(MessageType::Error, err);
            return entt::null;
      }

      return id;
}

void PfxContext::DestroyHandle(entt::entity handle) {
      _world.destroy(handle);
}

void PfxContext::CmdReset(entt::entity render_node) {
      if (!_world.valid(render_node)) {
            const auto err = "PfxContext::CmdBegin - Invalid handle";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      auto comp = _world.try_get<LoFi::Component::Ptx::RenderNode>(render_node);
      if (!comp) {
            const auto err = "PfxContext::CmdBegin - this handle is not a render node";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      comp->Reset();
}

void PfxContext::CmdGenerate(entt::entity render_node) {
      if (!_world.valid(render_node)) {
            const auto err = "PfxContext::CmdGenerate - Invalid handle";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      auto comp = _world.try_get<LoFi::Component::Ptx::RenderNode>(render_node);
      if (!comp) {
            const auto err = "PfxContext::CmdGenerate - this handle is not a render node";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      comp->Generate();
}

void PfxContext::CmdEmitCommand(entt::entity render_node, const std::optional<VkViewport>& viewport) const {
      if (!_world.valid(render_node)) {
            const auto err = "PfxContext::CmdEmitCommand - Invalid handle";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      auto comp = _world.try_get<LoFi::Component::Ptx::RenderNode>(render_node);
      if (!comp) {
            const auto err = "PfxContext::CmdEmitCommand - this handle is not a render node";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      comp->EmitCommand(viewport);
}

void PfxContext::CmdSetVirtualCanvasSize(entt::entity render_node, uint32_t w, uint32_t h) {
      if (!_world.valid(render_node)) {
            const auto err = "PfxContext::CmdSetVirtualCanvasSize - Invalid handle";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      auto comp = _world.try_get<LoFi::Component::Ptx::RenderNode>(render_node);
      if (!comp) {
            const auto err = "PfxContext::CmdSetVirtualCanvasSize - this handle is not a render node";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      comp->SetVirtualCanvasSize(w, h);
}

void PfxContext::CmdPushScissor(entt::entity render_node, int x, int y, uint32_t width, uint32_t height) {
      if (!_world.valid(render_node)) {
            const auto err = "PfxContext::CmdPushScissor - Invalid handle";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      auto comp = _world.try_get<LoFi::Component::Ptx::RenderNode>(render_node);
      if (!comp) {
            const auto err = "PfxContext::CmdPushScissor - this handle is not a render node";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      comp->PushScissor(x, y, width, height);
}

void PfxContext::CmdPopScissor(entt::entity render_node) {
      if (!_world.valid(render_node)) {
            const auto err = "PfxContext::CmdPopScissor - Invalid handle";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      auto comp = _world.try_get<LoFi::Component::Ptx::RenderNode>(render_node);
      if (!comp) {
            const auto err = "PfxContext::CmdPopScissor - this handle is not a render node";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      comp->PopScissor();
}

void PfxContext::CmdDrawRect(entt::entity render_node, uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t r, uint32_t g, uint32_t b, uint32_t a) {
      if (!_world.valid(render_node)) {
            const auto err = "PfxContext::CmdDrawRect - Invalid handle";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      auto comp = _world.try_get<LoFi::Component::Ptx::RenderNode>(render_node);
      if (!comp) {
            const auto err = "PfxContext::CmdDrawRect - this handle is not a render node";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      comp->DrawRect(x, y, width, height, r, g, b, a);
}

void PfxContext::CmdDrawPath(entt::entity render_node, std::span<glm::ivec2> pos, bool closed, uint32_t thickness, uint32_t r, uint32_t g, uint32_t b, uint32_t a, uint32_t expand) {
      if (!_world.valid(render_node)) {
            const auto err = "PfxContext::CmdDrawPath - Invalid handle";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      auto comp = _world.try_get<LoFi::Component::Ptx::RenderNode>(render_node);
      if (!comp) {
            const auto err = "PfxContext::CmdDrawPath - this handle is not a render node";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      comp->DrawPath(pos, closed, thickness, r, g, b, a, expand);
}

void PfxContext::CmdNewLayer(entt::entity render_node) {
      if (!_world.valid(render_node)) {
            const auto err = "PfxContext::CmdNewLayer - Invalid handle";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      auto comp = _world.try_get<LoFi::Component::Ptx::RenderNode>(render_node);
      if (!comp) {
            const auto err = "PfxContext::CmdNewLayer - this handle is not a render node";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }

      comp->NewLayer();
}

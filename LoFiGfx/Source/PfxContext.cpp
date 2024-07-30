//
// Created by Arzuo on 2024/7/9.
//

#include "PfxContext.h"
#include "GfxContext.h"
#include "Message.h"
#include "../Third/msdf-atlas-gen/msdf-atlas-gen.h"
#include "GfxComponents/Buffer.h"
#include "taskflow/algorithm/for_each.hpp"

using namespace LoFi;
using namespace msdf_atlas;

PfxContext::~PfxContext() = default;

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

      for (int i = 0; i < 3; i++) {
            _bufferVertex[i] = _gfx->CreateBuffer(8192, true);
            _bufferIndex[i] = _gfx->CreateBuffer(8192, true);

            _bufferInstance[i] = _gfx->CreateBuffer(8192, true);
            _bufferIndirect[i] = _gfx->CreateBuffer(8192, true);

            _bufferGradient[i] = _gfx->CreateBuffer(8192, true);
      }


      const auto draw_config = R"(
            #set rt = r8g8b8a8_unorm
            #set color_blend = src_alpha
            #set cull_mode = none
            //#set polygon_mode = line

            #set vs_location = 0 0 r32g32_sfloat 0
            #set vs_location = 0 1 r32g32_sfloat 8

            #set vs_location = 1 2 r32g32_uint 0
            #set vs_location = 1 3 r32_uint 8
            #set vs_location = 1 4 r32_uint 12
            #set vs_location = 1 5 r32g32_sfloat 16
            #set vs_location = 1 6 r32_sfloat 24
            #set vs_location = 1 7 r32_uint 28
            #set vs_location = 1 8 r32_uint 32
            #set vs_location = 1 9 r32_uint 36
            #set vs_location = 1 10 r32g32b32a32_sfloat 40
            #set vs_location = 1 11 r32g32b32a32_sfloat 56

            #set vs_binding = 0 16 vertex
            #set vs_binding = 1 72 instance
      )";

      const auto vs = R"(
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_buffer_reference : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_buffer_reference2 : enable

layout(location = 0) in vec2 in_Pos;
layout(location = 1) in vec2 UV;

layout(location = 2) in uvec2 Scissor;
layout(location = 3) in uint CanvasSize;
layout(location = 4) in uint Size;
layout(location = 5) in vec2 Center;
layout(location = 6) in float CenterRotate;
layout(location = 7) in uint Color;
layout(location = 8) in uint TextureBindlessIndex_or_GradientDataOffset;
layout(location = 9) in uint StrockType_PrimitiveType;
layout(location = 10) in vec4 Parameter1;
layout(location = 11) in vec4 Parameter2;


layout(location = 0)  out vec2          out_NDCPos;
layout(location = 1)  out vec2          out_OriginPos;
layout(location = 2)  out vec2          out_UV;
layout(location = 3)  out vec2          out_GlobalUV;

layout(location = 4)  out flat vec2     out_CanvasSize;
layout(location = 5)  out flat vec2     out_ScissorStart;
layout(location = 6)  out flat vec2     out_ScissorSize;

layout(location = 7)  out flat vec2     out_Size;
layout(location = 8)  out flat vec2     out_Center;
layout(location = 9)  out flat uint     out_Color;

layout(location = 11) out flat uint     out_TextureBindlessIndex_or_GradientDataOffset;
layout(location = 12) out flat uint     out_StrockType_PrimitiveType;
layout(location = 13) out flat vec4     out_Parameter1;
layout(location = 14) out flat vec4     out_Parameter2;

vec2 UnpackUintToVec2(uint packed) {
        float x = float(packed & 0xFFFF);
        float y = float((packed >> 16) & 0xFFFF);
        return vec2(x, y);
}

vec4 UnpackUintToColor(uint packed){
        return vec4(
                (packed & 0xFF) / 255.0f,
                ((packed >> 8) & 0xFF) / 255.0f,
                ((packed >> 16) & 0xFF) / 255.0f,
                ((packed >> 24) & 0xFF) / 255.0f
        );
}

void UnpackUintTo16(uint packed, out uint A, out uint B) {
        A  = (packed & 0xFFFF);
        B  = ((packed >> 16) & 0xFFFF);
}

void VSMain() {

        vec2 OriginPos = in_Pos;
        vec2 canvasSize = UnpackUintToVec2(CanvasSize);

        //Canvas Size to -1.0 ~ 1.0
        out_OriginPos = OriginPos;
        out_UV = UV;
        out_GlobalUV = OriginPos / canvasSize;

        float rotate_rad = radians(CenterRotate);
        mat2 rotateMat = mat2(cos(rotate_rad), -sin(rotate_rad), sin(rotate_rad), cos(rotate_rad));
        vec2 pos_center_rotated = rotateMat * (OriginPos - Center) + Center;

        out_NDCPos = (pos_center_rotated / canvasSize) * 2.0f - 1.0f;

        //rotate at center

        gl_Position = vec4(out_NDCPos, 0.0f, 1.0f);

        out_CanvasSize = canvasSize;
        vec2 scissorStart = UnpackUintToVec2(Scissor.x);
        vec2 scissorSize =  UnpackUintToVec2(Scissor.y);
        out_ScissorStart = scissorStart;
        out_ScissorSize = scissorSize;

        out_Size = UnpackUintToVec2(Size);
        out_Center = Center;
        out_Color = Color;

        out_TextureBindlessIndex_or_GradientDataOffset = TextureBindlessIndex_or_GradientDataOffset;
        out_StrockType_PrimitiveType = StrockType_PrimitiveType;
        out_Parameter1 = Parameter1;
        out_Parameter2 = Parameter2;
}
)";

      const auto fs = R"(
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_buffer_reference : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_buffer_reference2 : enable

layout(location = 0)  in vec2          in_NDCPos;
layout(location = 1)  in vec2          in_OriginPos;
layout(location = 2)  in vec2          in_UV;
layout(location = 3)  in vec2          in_GlobalUV;

layout(location = 4)  in flat vec2     in_CanvasSize;
layout(location = 5)  in flat vec2     in_ScissorStart;
layout(location = 6)  in flat vec2     in_ScissorSize;
layout(location = 7)  in flat vec2     in_Size;
layout(location = 8)  in flat vec2     in_Center;
layout(location = 9)  in flat uint     in_Color;
layout(location = 11) in flat uint     in_TextureBindlessIndex_or_GradientDataOffset;
layout(location = 12) in flat uint     in_StrockType_PrimitiveType;
layout(location = 13) in flat vec4     in_Parameter1;
layout(location = 14) in flat vec4     in_Parameter2;


layout(location = 0) out vec4 outColor;

vec4 UnpackUintToColor(uint packed){
        return vec4(
                (packed & 0xFF) / 255.0f,
                ((packed >> 8) & 0xFF) / 255.0f,
                ((packed >> 16) & 0xFF) / 255.0f,
                ((packed >> 24) & 0xFF) / 255.0f
        );
}

void UnpackUintTo16(uint packed, out uint A, out uint B) {
        A  = (packed & 0xFFFF);
        B  = ((packed >> 16) & 0xFFFF);
}


vec2 UnpackUintToVec2(uint packed) {
        float x = float(packed & 0xFFFF);
        float y = float((packed >> 16) & 0xFFFF);
        return vec2(x, y);
}


#define PI 3.14159265358

float sdBox(in vec2 p, in vec2 b ) {
    vec2 d = abs(p)-b;
    return length(max(d,0.0)) + min(max(d.x,d.y),0.0);
}

float sdRoundBox( in vec2 p, in vec2 b, in vec4 r )
{
    r.xy = (p.x>0.0)?r.xy : r.zw;
    r.x  = (p.y>0.0)?r.x  : r.y;
    vec2 q = abs(p)-b+r.x;
    return min(max(q.x,q.y),0.0) + length(max(q,0.0)) - r.x;
}

float sdNGon( in vec2 p, float r, float sides, float roundness) {
    // these 2 lines can be precomputed
    float an = 6.2831853/float(sides);
    float he = r*tan(0.5*an);
    p*= 1.0 + roundness/r;
    // rotate to first sector
    p = -p.yx; // if you want the corner to be up
    float bn = an*floor((atan(p.y,p.x)+0.5*an)/an);
    vec2  cs = vec2(cos(bn),sin(bn));
    p = mat2(cs.x,-cs.y,cs.y,cs.x)*p;
    // side of polygon
    return (length(p-vec2(r,clamp(p.y,-he,he)))*sign(p.x-r) - roundness) / (1.0 + roundness/r);
}

float sdCircle( vec2 p, float r )
{
    return length(p) - r;
}

void FSMain() {
    if(in_OriginPos.x < in_ScissorStart.x || in_OriginPos.x > in_ScissorStart.x + in_ScissorSize.x || in_OriginPos.y < in_ScissorStart.y || in_OriginPos.y > in_ScissorStart.y + in_ScissorSize.y) {
        discard;
    }

    uint StrockType;
    uint PrimitiveType;
    UnpackUintTo16(in_StrockType_PrimitiveType, StrockType, PrimitiveType);

    vec2 fragCoord = (in_OriginPos - in_Center);

    float aspect = in_Size.x / in_Size.y;

    float d = 0.0f;
    //fragCoord *= 1.05f;

    if(PrimitiveType == 0) { // Box
        vec2 boxc_size = in_Parameter1.xy;
        d = sdBox(fragCoord,  boxc_size / 2);

    } else if(PrimitiveType == 1) { // RoundBox
        vec2 boxc_size = in_Parameter1.xy;
        vec4 roundness = vec4(in_Parameter1.zw, in_Parameter2.xy);
        d = sdRoundBox(fragCoord,  boxc_size / 2, roundness * min(boxc_size.x, boxc_size.y) / 2);

    } else if(PrimitiveType == 2) { // RoundNGon
        float r = in_Parameter1.x;
        float sides = in_Parameter1.y;
        float roundness = in_Parameter1.z;
        d = sdNGon(fragCoord, r, sides, roundness * r);

    } else if(PrimitiveType == 3) { // Circle
        float r = in_Parameter1.x;
        d = sdCircle(fragCoord, r);

    } else {

    }

    vec4 finalColor;
    if(StrockType == 0){
        vec4 soild_color = UnpackUintToColor(in_Color);
        finalColor = soild_color;
    } else {
        vec3 col = (d>0.0) ? vec3(0.9,0.6,0.3) : vec3(0.65,0.85,1.0);
        //col *= 1.0 - exp(-0.02*abs(d)); //shadow
        col *= 0.8 + 0.3*cos(0.25*d);
        col = mix( col, vec3(1.0), 1.0-smoothstep(0.0,5,abs(d)) );
        finalColor = vec4(col, 1.0);
    }

    float d2 = d;
    //d2 = clamp(d2, 0, 1);
    d2 = smoothstep(5, 0.0, d);


    outColor = finalColor * vec4(1.0f, 1.0f, 1.0f, d2);
}

)";

      //_programDraw = _gfx->CreateProgramFromFile({"D://DDraw.fs", "D://DDraw.vs"}, draw_config);

      _programDraw = _gfx->CreateProgram({vs, fs}, draw_config);
      _kernelDraw = _gfx->CreateKernel(_programDraw);

      Reset();
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
                  _gfx->UploadTexture2D(_fontAtlas, bitmap.pixels, bitmap.width * bitmap.height * 4);

                  msdfgen::destroyFont(font);
            }
            msdfgen::deinitializeFreetype(ft);
      }

      return true;
}

void PfxContext::PushCanvasSize(glm::u16vec2 size) {
      _canvasSizeStack.emplace_back(size);
      _currentCanvasSize = size;
}

void PfxContext::PopCanvasSize() {
      if (_canvasSizeStack.size() <= 1) {
            const auto warning = "PfxContext::PopCanvasSize - Can't Pop Root CanvasSize";
            MessageManager::Log(MessageType::Warning, warning);
            return;
      }
      _canvasSizeStack.pop_back();
      _currentCanvasSize = _canvasSizeStack.back();
}

void PfxContext::PushScissor(glm::u16vec4 xywh) {
      _scissorStack.emplace_back(xywh);
      _currentScissor = xywh;
}

void PfxContext::PopScissor() {
      if (_scissorStack.size() <= 1) {
            const auto warning = "PfxContext::PopScissor - Can't Pop Root Scissor";
            MessageManager::Log(MessageType::Warning, warning);
            return;
      }
      _scissorStack.pop_back();
      _currentScissor = _scissorStack.back();
}

void PfxContext::PushShadow(const ShadowParameter& shadow_parameter) {
      _shadowStack.push_back(shadow_parameter);
}

void PfxContext::PopShadow() {
      if (!_shadowStack.empty()) {
            _shadowStack.pop_back();
      }
}

void PfxContext::PushStrock(const StrockTypeParameter& strock_parameter) {
      _strockStack.emplace_back(strock_parameter);
      _currentStrock = _strockStack.back();
}

void PfxContext::PopStrock() {
      if (_strockStack.size() <= 1) {
            const auto warning = "PfxContext::PopStrock - Can't Pop Root Strock";
            MessageManager::Log(MessageType::Warning, warning);
            return;
      }
      _strockStack.pop_back();
      _currentStrock = _strockStack.back();
}

void PfxContext::DrawBox(glm::vec2 start, PParamBox param, float rotation, glm::u8vec4 color) {
      uint32_t voffset = _vertexData.size();
      uint32_t ioffset = _indexData.size();
      uint32_t indirect_offset = _indirectData.size();

      _vertexData.reserve(voffset + 4);
      _indexData.reserve(ioffset + 6);

      const glm::vec2 size = param.Size;
      const glm::vec2 ex_size = size + glm::vec2(_pixelExpand * 2);

      _vertexData.emplace_back(start + glm::vec2(0, ex_size.y), glm::vec2(0, 1));
      _vertexData.emplace_back(start + ex_size, glm::vec2(1, 1));
      _vertexData.emplace_back(start, glm::vec2(0, 0));
      _vertexData.emplace_back(start + glm::vec2(ex_size.x, 0), glm::vec2(1, 0));

      _indexData.emplace_back(voffset + 0);
      _indexData.emplace_back(voffset + 1);
      _indexData.emplace_back(voffset + 2);
      _indexData.emplace_back(voffset + 2);
      _indexData.emplace_back(voffset + 1);
      _indexData.emplace_back(voffset + 3 );

      _indirectData.emplace_back(
             6,
             1,
             ioffset,
             0,
             indirect_offset);

      auto& ref = NewDrawInstanceData();
      ref.Color = color;
      ref.CenterRotate = rotation;
      ref.Size = ex_size;
      ref.Center = start + ex_size / 2.0f;
      ref.PrimitiveType = DrawPrimitveType::Box;
      ref.PrimitiveParameter.Box = param;
}

void PfxContext::DrawRoundBox(glm::vec2 start, PParamRoundBox param, float rotation, glm::u8vec4 color) {
      uint32_t voffset = _vertexData.size();
      uint32_t ioffset = _indexData.size();
      uint32_t indirect_offset = _indirectData.size();

      _vertexData.reserve(voffset + 4);
      _indexData.reserve(ioffset + 6);

      const glm::vec2 size = param.Size;
      const glm::vec2 ex_size = size + glm::vec2(_pixelExpand * 2);

      _vertexData.emplace_back(start + glm::vec2(0, ex_size.y), glm::vec2(0, 1));
      _vertexData.emplace_back(start + ex_size, glm::vec2(1, 1));
      _vertexData.emplace_back(start, glm::vec2(0, 0));
      _vertexData.emplace_back(start + glm::vec2(ex_size.x, 0), glm::vec2(1, 0));

      _indexData.emplace_back(voffset + 0);
      _indexData.emplace_back(voffset + 1);
      _indexData.emplace_back(voffset + 2);
      _indexData.emplace_back(voffset + 2);
      _indexData.emplace_back(voffset + 1);
      _indexData.emplace_back(voffset + 3);

      _indirectData.emplace_back(
             6,
             1,
             ioffset,
             0,
             indirect_offset);

      auto& ref = NewDrawInstanceData();
      ref.Color = color;
      ref.CenterRotate = rotation;
      ref.Size = ex_size;
      ref.Center = start + ex_size / 2.0f;
      ref.PrimitiveType = DrawPrimitveType::RoundBox;
      ref.PrimitiveParameter.RoundRect = param;
}

void PfxContext::DrawNGon(glm::vec2 center, PParamRoundNGon param, float rotation, glm::u8vec4 color) {
      uint32_t voffset = _vertexData.size();
      uint32_t ioffset = _indexData.size();
      uint32_t indirect_offset = _indirectData.size();

      _vertexData.reserve(voffset + 4);
      _indexData.reserve(ioffset + 6);

      const float r = param.Radius;

      float an = 6.2831853f / (param.SegmentCount) / 2;
      float he = r / cos(an);
      const float ex_size = he + _pixelExpand;

      _vertexData.emplace_back(center + glm::vec2(-ex_size, ex_size), glm::vec2(0, 1));
      _vertexData.emplace_back(center + ex_size, glm::vec2(1, 1));
      _vertexData.emplace_back(center + glm::vec2(-ex_size, -ex_size), glm::vec2(0, 0));
      _vertexData.emplace_back(center + glm::vec2(ex_size, -ex_size), glm::vec2(1, 0));

      _indexData.emplace_back(voffset + 0);
      _indexData.emplace_back(voffset + 1);
      _indexData.emplace_back(voffset + 2);
      _indexData.emplace_back(voffset + 2);
      _indexData.emplace_back(voffset + 1);
      _indexData.emplace_back(voffset + 3);

      _indirectData.emplace_back(
             6,
             1,
             ioffset,
             0,
             indirect_offset);

      auto& ref = NewDrawInstanceData();
      ref.Color = color;
      ref.CenterRotate = rotation;
      ref.CenterRotate = rotation;
      ref.Size = glm::vec2(ex_size, ex_size) * 2.0f;
      ref.Center = center;
      ref.PrimitiveType = DrawPrimitveType::RoundNGon;
      ref.PrimitiveParameter.RoundNGon = param;
}

void PfxContext::DrawCircle(glm::vec2 center, PParamCircle param, float rotation, glm::u8vec4 color) {
      uint32_t voffset = _vertexData.size();
      uint32_t ioffset = _indexData.size();
      uint32_t indirect_offset = _indirectData.size();

      _vertexData.reserve(voffset + 4);
      _indexData.reserve(ioffset + 6);

      const float r = param.Radius;
      const float ex_size = r + _pixelExpand;

      _vertexData.emplace_back(center + glm::vec2(-ex_size, ex_size), glm::vec2(0, 1));
      _vertexData.emplace_back(center + glm::vec2(ex_size,ex_size) , glm::vec2(1, 1));
      _vertexData.emplace_back(center + glm::vec2(-ex_size, -ex_size), glm::vec2(0, 0));
      _vertexData.emplace_back(center + glm::vec2(ex_size, -ex_size), glm::vec2(1, 0));

      _indexData.emplace_back(voffset + 0);
      _indexData.emplace_back(voffset + 1);
      _indexData.emplace_back(voffset + 2);
      _indexData.emplace_back(voffset + 2);
      _indexData.emplace_back(voffset + 1);
      _indexData.emplace_back(voffset + 3);

      _indirectData.emplace_back(
             6,
             1,
             ioffset,
             0,
             indirect_offset);

      auto& ref = NewDrawInstanceData();
      ref.Color = color;
      ref.CenterRotate = rotation;
      ref.Size = glm::vec2(ex_size, ex_size) * 2.0f;
      ref.Center = center;
      ref.PrimitiveType = DrawPrimitveType::Circle;
      ref.PrimitiveParameter.Circle = param;
}

auto PfxContext::EmitDrawCommand() -> void {
      if (_vertexData.empty()) return;
      auto current_index = _gfx->GetCurrentFrameIndex();

      for (auto i : _sampledImageReference[current_index]) {
            _gfx->AsSampledTexure(i, GRAPHICS);
      }

      _gfx->CmdBindKernel(_kernelDraw);

      _gfx->CmdBindVertexBuffer(_bufferVertex[current_index], 0);
      _gfx->CmdBindVertexBuffer(_bufferInstance[current_index], 1);
      _gfx->CmdBindIndexBuffer(_bufferIndex[current_index], 0);

      _gfx->CmdDrawIndexedIndirect(_bufferIndirect[current_index], 0, _indirectData.size(), sizeof(VkDrawIndexedIndirectCommand));
}

void PfxContext::Reset() {

      //rendering op clear
      _vertexData.clear();
      _indexData.clear();

      _instanceData.clear();
      _indirectData.clear();
      _gradientData.clear();

      _sampledImageReference[_gfx->GetCurrentFrameIndex()].clear();

      //init state
      _currentScissor = {0, 0, 3840, 2160};
      _currentCanvasSize = {3840, 2160};
      _currentStrock = {
            .Type = StrockFillType::Soild,
      };

      _canvasSizeStack.clear();
      _scissorStack.clear();
      _strockStack.clear();

      _scissorStack.emplace_back(_currentScissor);
      _canvasSizeStack.emplace_back( _currentCanvasSize);
      _strockStack.emplace_back(_currentStrock);

      _shadowStack.clear();
}

GenInstanceData& PfxContext::NewDrawInstanceData() {

      auto& current_instance_data = _instanceData.emplace_back();

      current_instance_data.Scissor = _currentScissor;
      current_instance_data.CanvasSize = _currentCanvasSize;

      current_instance_data.StrockType = _currentStrock.Type;
      switch (_currentStrock.Type) {
            case StrockFillType::Soild:
                  current_instance_data.Color = _currentStrock.Solid.SoildColor;
                  break;
            case StrockFillType::Texture:
                  current_instance_data.TextureBindlessIndex_or_GradientDataOffset = _gfx->GetTextureBindlessIndex(_currentStrock.Texture.ImageHandle);
                  _sampledImageReference[_gfx->GetCurrentFrameIndex()].insert(_currentStrock.Texture.ImageHandle);
                  break;
            case StrockFillType::Linear1:
            case StrockFillType::Linear2:
            case StrockFillType::Linear3:
            case StrockFillType::Linear4:
            case StrockFillType::Linear5:
                  current_instance_data.TextureBindlessIndex_or_GradientDataOffset = _gradientData.size();
                  _gradientData.emplace_back(
                         glm::vec2(0, 0),
                        _currentStrock.Linear.GradientDirection,
                        _currentStrock.Linear.Pos1,
                        _currentStrock.Linear.Pos2,
                        _currentStrock.Linear.Pos3,
                        _currentStrock.Linear.Pos4,
                        _currentStrock.Linear.Pos5,
                        _currentStrock.Linear.Color1,
                        _currentStrock.Linear.Color2,
                        _currentStrock.Linear.Color3,
                        _currentStrock.Linear.Color4,
                        _currentStrock.Linear.Color5
                  );
                  break;
            case StrockFillType::Radial1:
            case StrockFillType::Radial2:
            case StrockFillType::Radial3:
            case StrockFillType::Radial4:
            case StrockFillType::Radial5:
                  current_instance_data.TextureBindlessIndex_or_GradientDataOffset = _gradientData.size();
                  _gradientData.emplace_back(
                  glm::vec2(0, 0),
                  0,
                  _currentStrock.Radial.Pos1,
                  _currentStrock.Radial.Pos2,
                  _currentStrock.Radial.Pos3,
                  _currentStrock.Radial.Pos4,
                  _currentStrock.Radial.Pos5,
                  _currentStrock.Radial.Color1,
                  _currentStrock.Radial.Color2,
                  _currentStrock.Radial.Color3,
                  _currentStrock.Radial.Color4,
                  _currentStrock.Radial.Color5
                  );
                  break;
      }

      return current_instance_data;
}

void PfxContext::DispatchGenerateCommands() {
      if (_vertexData.empty()) return;
      //_taskFlowDispatch.clear();

      auto ptr_vbuf = _gfx->UnsafeFetch<Component::Gfx::Buffer>(_bufferVertex[_gfx->GetCurrentFrameIndex()]);
      auto ptr_ibuf = _gfx->UnsafeFetch<Component::Gfx::Buffer>(_bufferIndex[_gfx->GetCurrentFrameIndex()]);
      auto ptr_instancebuf = _gfx->UnsafeFetch<Component::Gfx::Buffer>(_bufferInstance[_gfx->GetCurrentFrameIndex()]);
      auto ptr_indirectbuf = _gfx->UnsafeFetch<Component::Gfx::Buffer>(_bufferIndirect[_gfx->GetCurrentFrameIndex()]);

      auto ptr_gradientbuf = _gfx->UnsafeFetch<Component::Gfx::Buffer>(_bufferGradient[_gfx->GetCurrentFrameIndex()]);

      // auto task_CmdDrawFanFilled = _taskFlowDispatch.for_each(_cmdFanFilled.begin(), _cmdFanFilled.end(), [ptr_vertex_arr, ptr_index_arr](const CmdDrawFanFilled& item) {
      // });

      ptr_vbuf->SetData(_vertexData.data(), _vertexData.size() * sizeof(GenDrawVertexData));
      ptr_ibuf->SetData(_indexData.data(), _indexData.size() * sizeof(uint32_t));

      ptr_instancebuf->SetData(_instanceData.data(), _instanceData.size() * sizeof(GenInstanceData));
      ptr_indirectbuf->SetData(_indirectData.data(), _indirectData.size() * sizeof(VkDrawIndexedIndirectCommand));

      if(!_gradientData.empty()) {
            ptr_gradientbuf->SetData(_gradientData.data(), _gradientData.size() * sizeof(GradientData));
      }
     // _taskExecutor.run(_taskFlowDispatch).wait();
}

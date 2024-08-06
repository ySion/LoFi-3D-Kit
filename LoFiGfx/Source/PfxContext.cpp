//
// Created by Arzuo on 2024/7/9.
//

#include <codecvt>
#include <fstream>
#include <future>
#include <sstream>
#include <cwctype>

#include "PfxContext.h"
#include "RenderNode.h"
#include "Message.h"
#include "../Third/msdfgen/msdfgen.h"
#include "../Third/msdfgen/msdfgen-ext.h"
#include "GfxComponents/Buffer.h"

using namespace LoFi;

PfxContext::~PfxContext() {
      for (int i = 0; i < 3; i++) {
            _gfx->DestroyHandle(_bufferVertex[i]);
            _gfx->DestroyHandle(_bufferIndex[i]);
            _gfx->DestroyHandle(_bufferInstance[i]);
            _gfx->DestroyHandle(_bufferIndirect[i]);
            _gfx->DestroyHandle(_bufferGradient[i]);
      }
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

      _fontDOT.insert(L',');
      _fontDOT.insert(L'.');
      _fontDOT.insert(L'_');


      for (int i = 0; i < 3; i++) {
            _bufferVertex[i] = _gfx->CreateBuffer({.pResourceName = "Pfx Vertex Buffer", .DataSize = 8192, .bCpuAccess = true});
            _bufferIndex[i] = _gfx->CreateBuffer({.pResourceName = "Pfx Index Buffer", .DataSize = 8192, .bCpuAccess = true});

            _bufferInstance[i] = _gfx->CreateBuffer({.pResourceName = "Pfx Instance Buffer", .DataSize = 8192, .bCpuAccess = true});
            _bufferIndirect[i] = _gfx->CreateBuffer({.pResourceName = "Pfx Indirect Buffer", .DataSize = 8192, .bCpuAccess = true});

            _bufferGradient[i] = _gfx->CreateBuffer({.pResourceName = "Pfx Gradient Buffer", .DataSize = 8192, .bCpuAccess = true});
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

      const char* fs = R"(
            #extension GL_EXT_nonuniform_qualifier : enable
            #extension GL_EXT_buffer_reference : enable
            #extension GL_EXT_scalar_block_layout : enable
            #extension GL_EXT_buffer_reference2 : enable


            layout(location = 0)  in vec2          in_PosAfterTransform;
            layout(location = 1)  in vec2          in_NDCPos;
            layout(location = 2)  in vec2          in_OriginPos;
            layout(location = 3)  in vec2          in_UV;
            layout(location = 4)  in vec2          in_GlobalUV;

            layout(location = 5)  in flat vec2     in_CanvasSize;
            layout(location = 6)  in flat vec2     in_ScissorStart;
            layout(location = 7)  in flat vec2     in_ScissorSize;
            layout(location = 8)  in flat vec2     in_Size;
            layout(location = 9)  in flat vec2     in_Center;
            layout(location = 10) in flat uint     in_Color;
            layout(location = 11) in flat uint     in_TextureBindlessIndex_or_GradientDataOffset;
            layout(location = 12) in flat uint     in_StrockType_PrimitiveType;
            layout(location = 13) in flat vec4     in_Parameter1;
            layout(location = 14) in flat vec4     in_Parameter2;

            layout(location = 0) out vec4 outColor;

            layout(binding = 0) uniform sampler2D TextureArray[];

            struct GradientParameter {
                vec2 Offset;
                float Angle;
                float Pos1;
                float Pos2;
                float Pos3;
                float Pos4;
                float Pos5;
                float Pos6;
                uint Color1;
                uint Color2;
                uint Color3;
                uint Color4;
                uint Color5;
                uint Color6;
            };

            layout(buffer_reference, scalar) buffer PtrGradientBufferArray {
                GradientParameter GradientData[];
            };

            layout(push_constant) uniform InfosType {
                PtrGradientBufferArray gradientArray;
            } Infos;

            vec4 UnpackUintToColor(uint packed){
                    return vec4(
                            (packed & 0xFF) / 255.0f,
                            ((packed >> 8) & 0xFF) / 255.0f,
                            ((packed >> 16) & 0xFF) / 255.0f,
                            ((packed >> 24) & 0xFF) / 255.0f
                    );
            }

            vec2 UnpackUintToVec2(uint packed) {
                    float x = float(packed & 0xFFFF);
                    float y = float((packed >> 16) & 0xFFFF);
                    return vec2(x, y);
            }

            void UnpackUintTo16(uint packed, out uint A, out uint B) {
                A  = (packed & 0xFFFF);
                B  = ((packed >> 16) & 0xFFFF);
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

            vec2 rot2(vec2 v, vec2 origin, float theta)
            {
                vec2 v2 = v - origin;
                return vec2(v2.x * cos(theta) - v2.y * sin(theta), v2.y * cos(theta) + v2.x * sin(theta)) + origin;
            }


            float median(float r, float g, float b) {
                return max(min(r, g), min(max(r, g), b));
            }


            float screenPxRange(float pxRange) {
                vec2 unitRange = vec2(pxRange)/vec2(800, 800);
                vec2 screenTexSize = vec2(1.0)/fwidth(in_UV);
                return max(0.5*dot(unitRange, screenTexSize), 1.0);
            }

            vec4 renderFont(vec3 msdf, vec4 bgColor, vec4 fgColor, float pxRange) {
                float sd = median(msdf.r, msdf.g, msdf.b);
                float screenPxDistance = pxRange*(sd - 0.5);
                float opacity = clamp(screenPxDistance + 0.8, 0.0, 1.0);
                return mix(bgColor, fgColor, opacity);
            }


            void FSMain() {
                if(in_PosAfterTransform.x < in_ScissorStart.x || in_PosAfterTransform.x > in_ScissorStart.x + in_ScissorSize.x || in_PosAfterTransform.y < in_ScissorStart.y || in_PosAfterTransform.y > in_ScissorStart.y + in_ScissorSize.y) {
                    discard;
                }

                uint StrockType;
                uint PrimitiveType;
                UnpackUintTo16(in_StrockType_PrimitiveType, StrockType, PrimitiveType);

                vec2 fragCoord = (in_OriginPos - in_Center);

                float aspect = in_Size.x / in_Size.y;
                float aspectInver = in_Size.y / in_Size.x;

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

                } else if(PrimitiveType == 50) { //Text
                    vec2 uv = in_UV;
                    float pxRange = in_Parameter1.z;
                    vec3 msdf = texture(TextureArray[in_TextureBindlessIndex_or_GradientDataOffset], uv).rgb;
                    //float pxRange = 2; // font_piece_canvas_resolution  / sdf_part_resolution * 2
                    //d = median(msdf.r, msdf.g, msdf.b);
                    //float screenPxDistance = pxRange*(d - 0.35);
                    //float opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
                    //outColor = mix(vec4(1.0f, 1.0f, 1.0f, 0.0f), vec4(0.0f, 0.0f, 0.0f, 1.0f), opacity);
                    outColor = renderFont(msdf, vec4(1.0f, 1.0f, 1.0f, 0.0f), vec4(0.0f, 0.0f, 0.0f, 1.0f), pxRange);
                    //outColor = vec4(msdf.rrr, 1.0f);
                }

                if(PrimitiveType != 50){
                    vec4 finalColor;
                    if(StrockType == 0){ // Soild
                        vec4 soild_color = UnpackUintToColor(in_Color);
                        finalColor = soild_color;
                    } else if(StrockType == 1){ // Texture
                        vec2 uv = in_UV;
                        uv.y *= aspectInver;
                        uv.y = 1.0 - uv.y;
                        vec3 c = texture(TextureArray[in_TextureBindlessIndex_or_GradientDataOffset], uv).rgb;
                        finalColor = vec4(c, 1.0f);

                    } else if(StrockType >= 20) { // Radial2

                        if(StrockType == 20) {
                            GradientParameter gradientParameter = Infos.gradientArray.GradientData[in_TextureBindlessIndex_or_GradientDataOffset];
                            vec2 center_pos =  in_UV - vec2(0.5, 0.5)+ gradientParameter.Offset;

                            float Pos1 = gradientParameter.Pos1 * 0.5;
                            float Pos2 = gradientParameter.Pos2 * 0.5;

                            vec4 color1 = UnpackUintToColor(gradientParameter.Color1);
                            vec4 color2 = UnpackUintToColor(gradientParameter.Color2);

                            center_pos.y *= aspectInver;

                            float dis = length(center_pos);
                            finalColor = mix(color1, color2, clamp((dis -  Pos1) / (Pos2 - Pos1), 0.0f, 1.0f));

                        } else  if(StrockType == 21) {
                            GradientParameter gradientParameter = Infos.gradientArray.GradientData[in_TextureBindlessIndex_or_GradientDataOffset];
                            vec2 center_pos =  in_UV - vec2(0.5, 0.5)+ gradientParameter.Offset;

                            float Pos1 = gradientParameter.Pos1 * 0.5;
                            float Pos2 = gradientParameter.Pos2 * 0.5;
                            float Pos3 = gradientParameter.Pos3 * 0.5;

                            vec4 color1 = UnpackUintToColor(gradientParameter.Color1);
                            vec4 color2 = UnpackUintToColor(gradientParameter.Color2);
                            vec4 color3 = UnpackUintToColor(gradientParameter.Color3);

                            center_pos.y *= aspectInver;

                            float dis = length(center_pos);
                            finalColor = mix(color1, color2, clamp((dis -  Pos1) / (Pos2 - Pos1), 0.0f, 1.0f));
                            finalColor = mix(finalColor,  color3, clamp((dis -  Pos2)/(Pos3 - Pos2), 0.0, 1.0));


                        } else  if(StrockType == 22) {
                            GradientParameter gradientParameter = Infos.gradientArray.GradientData[in_TextureBindlessIndex_or_GradientDataOffset];
                            vec2 center_pos =  in_UV - vec2(0.5, 0.5)+ gradientParameter.Offset;

                            float Pos1 = gradientParameter.Pos1 * 0.5;
                            float Pos2 = gradientParameter.Pos2 * 0.5;
                            float Pos3 = gradientParameter.Pos3 * 0.5;
                            float Pos4 = gradientParameter.Pos4 * 0.5;

                            vec4 color1 = UnpackUintToColor(gradientParameter.Color1);
                            vec4 color2 = UnpackUintToColor(gradientParameter.Color2);
                            vec4 color3 = UnpackUintToColor(gradientParameter.Color3);
                            vec4 color4 = UnpackUintToColor(gradientParameter.Color4);

                            center_pos.y *= aspectInver;

                            float dis = length(center_pos);
                            finalColor = mix(color1, color2, clamp((dis -  Pos1) / (Pos2 - Pos1), 0.0f, 1.0f));
                            finalColor = mix(finalColor,  color3, clamp((dis -  Pos2)/(Pos3 - Pos2), 0.0, 1.0));
                            finalColor = mix(finalColor,  color4, clamp((dis -  Pos3)/(Pos4 - Pos3), 0.0, 1.0));

                        } else if(StrockType == 23) {
                            GradientParameter gradientParameter = Infos.gradientArray.GradientData[in_TextureBindlessIndex_or_GradientDataOffset];
                            vec2 center_pos =  in_UV - vec2(0.5, 0.5)+ gradientParameter.Offset;

                            float Pos1 = gradientParameter.Pos1 * 0.5;
                            float Pos2 = gradientParameter.Pos2 * 0.5;
                            float Pos3 = gradientParameter.Pos3 * 0.5;
                            float Pos4 = gradientParameter.Pos4 * 0.5;
                            float Pos5 = gradientParameter.Pos5* 0.5;

                            vec4 color1 = UnpackUintToColor(gradientParameter.Color1);
                            vec4 color2 = UnpackUintToColor(gradientParameter.Color2);
                            vec4 color3 = UnpackUintToColor(gradientParameter.Color3);
                            vec4 color4 = UnpackUintToColor(gradientParameter.Color4);
                            vec4 color5 = UnpackUintToColor(gradientParameter.Color5);

                            center_pos.y *= aspectInver;

                            float dis = length(center_pos);
                            finalColor = mix(color1, color2, clamp((dis -  Pos1) / (Pos2 - Pos1), 0.0f, 1.0f));
                            finalColor = mix(finalColor,  color3, clamp((dis -  Pos2)/(Pos3 - Pos2), 0.0, 1.0));
                            finalColor = mix(finalColor,  color4, clamp((dis -  Pos3)/(Pos4 - Pos3), 0.0, 1.0));
                            finalColor = mix(finalColor,  color5, clamp((dis -  Pos4)/(Pos5 - Pos4), 0.0, 1.0));

                        } else if(StrockType == 24) {
                            GradientParameter gradientParameter = Infos.gradientArray.GradientData[in_TextureBindlessIndex_or_GradientDataOffset];
                            vec2 center_pos =  in_UV - vec2(0.5, 0.5)+ gradientParameter.Offset;

                            float Pos1 = gradientParameter.Pos1 * 0.5;
                            float Pos2 = gradientParameter.Pos2 * 0.5;
                            float Pos3 = gradientParameter.Pos3 * 0.5;
                            float Pos4 = gradientParameter.Pos4 * 0.5;
                            float Pos5 = gradientParameter.Pos5* 0.5;
                            float Pos6 = gradientParameter.Pos6* 0.5;

                            vec4 color1 = UnpackUintToColor(gradientParameter.Color1);
                            vec4 color2 = UnpackUintToColor(gradientParameter.Color2);
                            vec4 color3 = UnpackUintToColor(gradientParameter.Color3);
                            vec4 color4 = UnpackUintToColor(gradientParameter.Color4);
                            vec4 color5 = UnpackUintToColor(gradientParameter.Color5);
                            vec4 color6 = UnpackUintToColor(gradientParameter.Color6);

                            center_pos.y *= aspectInver;

                            float dis = length(center_pos);
                            finalColor = mix(color1, color2, clamp((dis -  Pos1) / (Pos2 - Pos1), 0.0f, 1.0f));
                            finalColor = mix(finalColor,  color3, clamp((dis -  Pos2)/(Pos3 - Pos2), 0.0, 1.0));
                            finalColor = mix(finalColor,  color4, clamp((dis -  Pos3)/(Pos4 - Pos3), 0.0, 1.0));
                            finalColor = mix(finalColor,  color5, clamp((dis -  Pos4)/(Pos5 - Pos4), 0.0, 1.0));
                            finalColor = mix(finalColor,  color6, clamp((dis -  Pos5)/(Pos6 - Pos5), 0.0, 1.0));
                        }

                    } else if(StrockType >= 10){ // Linear 2
                        if(StrockType == 10){
                            GradientParameter gradientParameter = Infos.gradientArray.GradientData[in_TextureBindlessIndex_or_GradientDataOffset];
                            float currentAngle = gradientParameter.Angle;
                            vec2 uv = rot2(in_UV, vec2(0.5, 0.5) , radians(currentAngle));

                            float Pos1 = gradientParameter.Pos1;
                            float Pos2 = gradientParameter.Pos2;

                            vec4 color1 = UnpackUintToColor(gradientParameter.Color1);
                            vec4 color2 = UnpackUintToColor(gradientParameter.Color2);

                            finalColor = mix(color1, color2, clamp((uv.x -  Pos1) / (Pos2 - Pos1), 0.0f, 1.0f));

                        } else if(StrockType == 11){
                            GradientParameter gradientParameter = Infos.gradientArray.GradientData[in_TextureBindlessIndex_or_GradientDataOffset];
                            float currentAngle = gradientParameter.Angle;
                            vec2 uv = rot2(in_UV, vec2(0.5, 0.5) , radians(currentAngle));

                            float Pos1 = gradientParameter.Pos1;
                            float Pos2 = gradientParameter.Pos2;
                            float Pos3 = gradientParameter.Pos3;

                            vec4 color1 = UnpackUintToColor(gradientParameter.Color1);
                            vec4 color2 = UnpackUintToColor(gradientParameter.Color2);
                            vec4 color3 = UnpackUintToColor(gradientParameter.Color3);

                            finalColor = mix(color1,     color2, clamp((uv.x -  Pos1) / (Pos2 - Pos1), 0.0f, 1.0f));
                            finalColor = mix(finalColor, color3, clamp((uv.x -  Pos2) / (Pos3 - Pos2), 0.0f, 1.0f));

                        } else if(StrockType == 12){
                            GradientParameter gradientParameter = Infos.gradientArray.GradientData[in_TextureBindlessIndex_or_GradientDataOffset];
                            float currentAngle = gradientParameter.Angle;
                            vec2 uv = rot2(in_UV, vec2(0.5, 0.5) , radians(currentAngle));

                            float Pos1 = gradientParameter.Pos1;
                            float Pos2 = gradientParameter.Pos2;
                            float Pos3 = gradientParameter.Pos3;
                            float Pos4 = gradientParameter.Pos4;

                            vec4 color1 = UnpackUintToColor(gradientParameter.Color1);
                            vec4 color2 = UnpackUintToColor(gradientParameter.Color2);
                            vec4 color3 = UnpackUintToColor(gradientParameter.Color3);
                            vec4 color4 = UnpackUintToColor(gradientParameter.Color4);

                            finalColor = mix(color1,     color2, clamp((uv.x -  Pos1) / (Pos2 - Pos1), 0.0f, 1.0f));
                            finalColor = mix(finalColor, color3, clamp((uv.x -  Pos2) / (Pos3 - Pos2), 0.0f, 1.0f));
                            finalColor = mix(finalColor, color4, clamp((uv.x -  Pos3) / (Pos4 - Pos3), 0.0f, 1.0f));
                        } else if(StrockType == 13){
                            GradientParameter gradientParameter = Infos.gradientArray.GradientData[in_TextureBindlessIndex_or_GradientDataOffset];
                            float currentAngle = gradientParameter.Angle;
                            vec2 uv = rot2(in_UV, vec2(0.5, 0.5) , radians(currentAngle));

                            float Pos1 = gradientParameter.Pos1;
                            float Pos2 = gradientParameter.Pos2;
                            float Pos3 = gradientParameter.Pos3;
                            float Pos4 = gradientParameter.Pos4;
                            float Pos5 = gradientParameter.Pos5;

                            vec4 color1 = UnpackUintToColor(gradientParameter.Color1);
                            vec4 color2 = UnpackUintToColor(gradientParameter.Color2);
                            vec4 color3 = UnpackUintToColor(gradientParameter.Color3);
                            vec4 color4 = UnpackUintToColor(gradientParameter.Color4);
                            vec4 color5 = UnpackUintToColor(gradientParameter.Color5);

                            finalColor = mix(color1,     color2, clamp((uv.x -  Pos1) / (Pos2 - Pos1), 0.0f, 1.0f));
                            finalColor = mix(finalColor, color3, clamp((uv.x -  Pos2) / (Pos3 - Pos2), 0.0f, 1.0f));
                            finalColor = mix(finalColor, color4, clamp((uv.x -  Pos3) / (Pos4 - Pos3), 0.0f, 1.0f));
                            finalColor = mix(finalColor, color5, clamp((uv.x -  Pos4) / (Pos5 - Pos4), 0.0f, 1.0f));
                        } else if(StrockType == 14){
                            GradientParameter gradientParameter = Infos.gradientArray.GradientData[in_TextureBindlessIndex_or_GradientDataOffset];
                            float currentAngle = gradientParameter.Angle;
                            vec2 uv = rot2(in_UV, vec2(0.5, 0.5) , radians(currentAngle));

                            float Pos1 = gradientParameter.Pos1;
                            float Pos2 = gradientParameter.Pos2;
                            float Pos3 = gradientParameter.Pos3;
                            float Pos4 = gradientParameter.Pos4;
                            float Pos5 = gradientParameter.Pos5;
                            float Pos6 = gradientParameter.Pos6;

                            vec4 color1 = UnpackUintToColor(gradientParameter.Color1);
                            vec4 color2 = UnpackUintToColor(gradientParameter.Color2);
                            vec4 color3 = UnpackUintToColor(gradientParameter.Color3);
                            vec4 color4 = UnpackUintToColor(gradientParameter.Color4);
                            vec4 color5 = UnpackUintToColor(gradientParameter.Color5);
                            vec4 color6 = UnpackUintToColor(gradientParameter.Color6);

                            finalColor = mix(color1,     color2, clamp((uv.x -  Pos1) / (Pos2 - Pos1), 0.0f, 1.0f));
                            finalColor = mix(finalColor, color3, clamp((uv.x -  Pos2) / (Pos3 - Pos2), 0.0f, 1.0f));
                            finalColor = mix(finalColor, color4, clamp((uv.x -  Pos3) / (Pos4 - Pos3), 0.0f, 1.0f));
                            finalColor = mix(finalColor, color5, clamp((uv.x -  Pos4) / (Pos5 - Pos4), 0.0f, 1.0f));
                            finalColor = mix(finalColor, color6, clamp((uv.x -  Pos5) / (Pos6 - Pos5), 0.0f, 1.0f));
                        }

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
            }

      )";

      const char* vs = R"(
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


            layout(location = 0)  out vec2          out_PosAfterTransform;
            layout(location = 1)  out vec2          out_NDCPos;
            layout(location = 2)  out vec2          out_OriginPos;
            layout(location = 3)  out vec2          out_UV;
            layout(location = 4)  out vec2          out_GlobalUV;

            layout(location = 5)  out flat vec2     out_CanvasSize;
            layout(location = 6)  out flat vec2     out_ScissorStart;
            layout(location = 7)  out flat vec2     out_ScissorSize;

            layout(location = 8)  out flat vec2     out_Size;
            layout(location = 9)  out flat vec2     out_Center;
            layout(location = 10)  out flat uint     out_Color;

            layout(location = 11) out flat uint     out_TextureBindlessIndex_or_GradientDataOffset;
            layout(location = 12) out flat uint     out_StrockType_PrimitiveType;
            layout(location = 13) out flat vec4     out_Parameter1;
            layout(location = 14) out flat vec4     out_Parameter2;

            struct GradientParameter {
                vec2 Offset;
                float Angle;
                float Pos1;
                float Pos2;
                float Pos3;
                float Pos4;
                float Pos5;
                float Pos6;
                uint Color1;
                uint Color2;
                uint Color3;
                uint Color4;
                uint Color5;
                uint Color6;
            };

            layout(buffer_reference, scalar) buffer PtrGradientBufferArray {
                GradientParameter GradientData[];
            };

            layout(push_constant) uniform InfosType {
                PtrGradientBufferArray gradientArray;
            } Infos;

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

                    out_PosAfterTransform = pos_center_rotated;

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


      const char* path[2] = {vs, fs};

      // _programDraw = _gfx->CreateProgramFromFile({
      //       .pResourceName = "PfxContext-Draw-Program",
      //       .pConfig = draw_config,
      //       .pSourceCodeFileNames = path,
      //       .countSourceCodeFileName = 2
      // });
      _programDraw = _gfx->CreateProgram({
            .pResourceName = "PfxContext-Draw-Program",
            .pConfig = draw_config,
            .pSourceCodes = path,
            .countSourceCode = 2
      });

      _kernelDraw = _gfx->CreateKernel(_programDraw, {.pResourceName = "PfxContext-Draw-Kernel"});
      if (_kernelDraw.Type != GfxEnumResourceType::Kernel) {
            const auto err = "[PfxContext::PfxContext] Can't Create Draw Kernel";
            MessageManager::Log(MessageType::Error, err);
            throw std::runtime_error(err);
      }
      Reset();
}

static uint8_t PixelFloatToByte(float x) {
      return static_cast<uint8_t>(~static_cast<int>(255.5f - 255.f * glm::clamp(x, 0.0f, 1.0f)));
}

static float PixelByteToFloat(uint8_t x) {
      return 1.f / 255.f * static_cast<float>(x);
}

bool PfxContext::GenAndLoadFont(const char* path) {
      if (msdfgen::FreetypeHandle* ft = msdfgen::initializeFreetype()) {
            if (msdfgen::FontHandle* font = msdfgen::loadFont(ft, path); font) {
                  const auto font_dataset_path = "f.txt";
                  std::locale locale(std::locale::classic(), new std::codecvt_utf8<wchar_t>);
                  std::wifstream file(font_dataset_path);

                  if (file.is_open()) {
                        file.imbue(locale);
                        std::wstringstream fs{};
                        while (fs << file.rdbuf()) {}
                        file.close();

                        std::wstring font_table = fs.str();

                        msdfgen::Bitmap<float, 4> bitmap(3200, 3200);

                        constexpr auto split = 250;
                        uint32_t threads = font_table.size() / split;

                        std::vector<std::future<void>> handles{};
                        printf("Star Gen %d th", threads);

                        std::vector<msdfgen::Shape> shapes{};

                        double global_size = 99999.0f;
                        double total_bottom_offset = 99999.0f;
                        for (int32_t i = 0; i < font_table.size(); i++) {
                              msdfgen::Shape shape{};
                              msdfgen::loadGlyph(shape, font, font_table[i], msdfgen::FONT_SCALING_LEGACY);
                              msdfgen::edgeColoringSimple(shape, 3.0);
                              shape.normalize();
                              auto bounds = shape.getBounds();

                              uint32_t x = i % 100;
                              uint32_t y = i / 100;

                              uint32_t start_pixel_x = x * 32;
                              uint32_t start_pixel_y = y * 32;

                              //printf("Bound %f %f %f %f\n", bounds.l, bounds.b, bounds.r, bounds.t);

                              float pixel_uvx = (float)start_pixel_x / 3200.0f;
                              float pixel_uvy = (float)start_pixel_y / 3200.0f;
                              float pixel_uvw = 32.0f / 3200.0f;
                              float pixel_uvh = 32.0f / 3200.0f;
                              if (_fontDOT.contains(font_table[i])) {
                                    pixel_uvw = 16.0f / 3200.0f;
                                    pixel_uvh = 16.0f / 3200.0f;
                              } else if (std::isalpha(font_table[i]) || std::isdigit(font_table[i]) || std::isspace(font_table[i]) || std::ispunct(font_table[i])) {
                                    pixel_uvw = 16.0f / 3200.0f;
                              }

                              _fontUVs[font_table[i]] = {pixel_uvx, pixel_uvy, pixel_uvw, pixel_uvh};


                              msdfgen::Vector2 frame(32, 32);

                              msdfgen::Range pxRange(4);
                              frame += 2 * pxRange.lower;
                              msdfgen::Vector2 dims(bounds.r - bounds.l, bounds.t - bounds.b);
                              if (bounds.l >= bounds.r || bounds.b >= bounds.t)
                                    bounds.l = 0, bounds.b = 0, bounds.r = 1, bounds.t = 1;

                              total_bottom_offset = glm::min(total_bottom_offset, bounds.b);

                              if (dims.x * frame.y < dims.y * frame.x) {
                                    if (frame.y / dims.y >= 0.0001) {
                                          global_size = glm::min(global_size, frame.y / dims.y);
                                          printf("Global %lf, Bound %lf %lf %lf %lf, size %lf ;\n", global_size, bounds.l, bounds.b, bounds.r, bounds.t, frame.y / dims.y);
                                    }
                              } else {
                                    if (frame.x / dims.x >= 0.0001) {
                                          global_size = glm::min(global_size, frame.x / dims.x);
                                          printf("Global %lf, Bound %lf %lf %lf %lf, size %lf ;\n", global_size, bounds.l, bounds.b, bounds.r, bounds.t, frame.x / dims.x);
                                    }
                              }
                              shapes.emplace_back(std::move(shape));
                        }

                        for (int32_t th = 0; th < threads; th++) {
                              uint32_t g_start_i = th * split;
                              uint32_t g_end_i = (th + 1) * split;
                              handles.emplace_back(std::async([&](uint32_t start_i, uint32_t end_i) {
                                    msdfgen::Bitmap<float, 4> MTSDF(32, 32);
                                    msdfgen::Bitmap<float, 4> MTSDF_ASCII(16, 32);
                                    msdfgen::Bitmap<float, 4> MTSDF_Dot(16, 16);
                                    for (uint32_t i = start_i; i < end_i; i++) {
                                          uint32_t x = i % 100;
                                          uint32_t y = i / 100;

                                          uint32_t start_pixel_x = x * 32;
                                          uint32_t start_pixel_y = y * 32;

                                          msdfgen::Vector2 frame(0, 0);
                                          if (_fontDOT.contains(font_table[i])) {
                                                frame = msdfgen::Vector2(10, 10);
                                          } else if (std::isalpha(font_table[i]) || std::isdigit(font_table[i]) || std::isspace(font_table[i]) || std::ispunct(font_table[i])) {
                                                frame = msdfgen::Vector2(16, 32);
                                          } else {
                                                frame = msdfgen::Vector2(32, 32);
                                          }

                                          msdfgen::Shape& shape = shapes[i];
                                          auto bounds = shape.getBounds();
                                          msdfgen::Range pxRange(4);
                                          frame += 2 * pxRange.lower;
                                          msdfgen::Vector2 translate;
                                          msdfgen::Vector2 scale;
                                          msdfgen::Vector2 dims(bounds.r - bounds.l, bounds.t - bounds.b);
                                          if (bounds.l >= bounds.r || bounds.b >= bounds.t)
                                                bounds.l = 0, bounds.b = 0, bounds.r = 1, bounds.t = 1;

                                          if (dims.x * frame.y < dims.y * frame.x) {
                                                translate.set(.5 * (frame.x / frame.y * dims.y - dims.x) - bounds.l, -total_bottom_offset); // -bounds.b
                                                scale = global_size;
                                          } else {
                                                translate.set(-bounds.l, -total_bottom_offset); //5*(frame.y/frame.x*dims.x-dims.y)-bounds.b
                                                scale = global_size;
                                          }
                                          translate -= pxRange.lower / scale;
                                          msdfgen::Range range = pxRange / glm::min(scale.x, scale.y);
                                          msdfgen::SDFTransformation ts(msdfgen::Projection(scale, translate), range);

                                          if (_fontDOT.contains(font_table[i])) {
                                                msdfgen::generateMTSDF(MTSDF_Dot, shape, ts);
                                                for (int j = 0; j < 16; j++) {
                                                      for (int k = 0; k < 16; k++) {
                                                            bitmap((int)(start_pixel_x + j), (int)(start_pixel_y + k))[0] = MTSDF_Dot(j, k)[0];
                                                            bitmap((int)(start_pixel_x + j), (int)(start_pixel_y + k))[1] = MTSDF_Dot(j, k)[1];
                                                            bitmap((int)(start_pixel_x + j), (int)(start_pixel_y + k))[2] = MTSDF_Dot(j, k)[2];
                                                            bitmap((int)(start_pixel_x + j), (int)(start_pixel_y + k))[3] = MTSDF_Dot(j, k)[3];
                                                      }
                                                }
                                          } else if (std::isalpha(font_table[i]) || std::isdigit(font_table[i]) || std::isspace(font_table[i]) || std::ispunct(font_table[i])) {
                                                msdfgen::generateMTSDF(MTSDF_ASCII, shape, ts);
                                                for (int j = 0; j < 16; j++) {
                                                      for (int k = 0; k < 32; k++) {
                                                            bitmap((int)(start_pixel_x + j), (int)(start_pixel_y + k))[0] = MTSDF_ASCII(j, k)[0];
                                                            bitmap((int)(start_pixel_x + j), (int)(start_pixel_y + k))[1] = MTSDF_ASCII(j, k)[1];
                                                            bitmap((int)(start_pixel_x + j), (int)(start_pixel_y + k))[2] = MTSDF_ASCII(j, k)[2];
                                                            bitmap((int)(start_pixel_x + j), (int)(start_pixel_y + k))[3] = MTSDF_ASCII(j, k)[3];
                                                      }
                                                }
                                          } else {
                                                msdfgen::generateMTSDF(MTSDF, shape, ts);
                                                for (int j = 0; j < 32; j++) {
                                                      for (int k = 0; k < 32; k++) {
                                                            bitmap((int)(start_pixel_x + j), (int)(start_pixel_y + k))[0] = MTSDF(j, k)[0];
                                                            bitmap((int)(start_pixel_x + j), (int)(start_pixel_y + k))[1] = MTSDF(j, k)[1];
                                                            bitmap((int)(start_pixel_x + j), (int)(start_pixel_y + k))[2] = MTSDF(j, k)[2];
                                                            bitmap((int)(start_pixel_x + j), (int)(start_pixel_y + k))[3] = MTSDF(j, k)[3];
                                                      }
                                                }
                                          }
                                    }
                              }, g_start_i, g_end_i));
                        }

                        {
                              uint32_t left = font_table.size() % split;

                              msdfgen::Bitmap<float, 4> MTSDF(32, 32);
                              msdfgen::Bitmap<float, 4> MTSDF_ASCII(16, 32);
                              msdfgen::Bitmap<float, 4> MTSDF_Dot(16, 16);

                              for (uint32_t i = threads * split; i < threads * split + left; i++) {
                                    uint32_t x = i % 100;
                                    uint32_t y = i / 100;

                                    uint32_t start_pixel_x = x * 32;
                                    uint32_t start_pixel_y = y * 32;

                                    msdfgen::Vector2 frame;
                                    if (_fontDOT.contains(font_table[i])) {
                                          frame = msdfgen::Vector2(10, 10);
                                    } else if (std::isalpha(font_table[i]) || std::isdigit(font_table[i]) || std::isspace(font_table[i]) || std::ispunct(font_table[i])) {
                                          frame = msdfgen::Vector2(16, 32);
                                    } else {
                                          frame = msdfgen::Vector2(32, 32);
                                    }

                                    msdfgen::Shape& shape = shapes[i];
                                    auto bounds = shape.getBounds();
                                    msdfgen::Range pxRange(4);
                                    frame += 2 * pxRange.lower;
                                    msdfgen::Vector2 translate;
                                    msdfgen::Vector2 scale;
                                    msdfgen::Vector2 dims(bounds.r - bounds.l, bounds.t - bounds.b);

                                    if (dims.x * frame.y < dims.y * frame.x) {
                                          translate.set(.5 * (frame.x / frame.y * dims.y - dims.x) - bounds.l, -bounds.b);
                                          scale = global_size; //frame.y/dims.y;
                                    } else {
                                          translate.set(-bounds.l, .5 * (frame.y / frame.x * dims.x - dims.y) - bounds.b);
                                          scale = global_size; //frame.x/dims.x;
                                    }

                                    translate -= pxRange.lower / scale;
                                    msdfgen::Range range = pxRange / glm::min(scale.x, scale.y);
                                    msdfgen::SDFTransformation ts(msdfgen::Projection(scale, translate), range);

                                    if (_fontDOT.contains(font_table[i])) {
                                          msdfgen::generateMTSDF(MTSDF_Dot, shape, ts);
                                          for (int j = 0; j < 16; j++) {
                                                for (int k = 0; k < 16; k++) {
                                                      bitmap((int)(start_pixel_x + j), (int)(start_pixel_y + k))[0] = MTSDF_Dot(j, k)[0];
                                                      bitmap((int)(start_pixel_x + j), (int)(start_pixel_y + k))[1] = MTSDF_Dot(j, k)[1];
                                                      bitmap((int)(start_pixel_x + j), (int)(start_pixel_y + k))[2] = MTSDF_Dot(j, k)[2];
                                                      bitmap((int)(start_pixel_x + j), (int)(start_pixel_y + k))[3] = MTSDF_Dot(j, k)[3];
                                                }
                                          }
                                    } else if (std::isalpha(font_table[i]) || std::isdigit(font_table[i]) || std::isspace(font_table[i]) || std::ispunct(font_table[i])) {
                                          msdfgen::generateMTSDF(MTSDF_ASCII, shape, ts);
                                          for (int j = 0; j < 16; j++) {
                                                for (int k = 0; k < 32; k++) {
                                                      bitmap((int)(start_pixel_x + j), (int)(start_pixel_y + k))[0] = MTSDF_ASCII(j, k)[0];
                                                      bitmap((int)(start_pixel_x + j), (int)(start_pixel_y + k))[1] = MTSDF_ASCII(j, k)[1];
                                                      bitmap((int)(start_pixel_x + j), (int)(start_pixel_y + k))[2] = MTSDF_ASCII(j, k)[2];
                                                      bitmap((int)(start_pixel_x + j), (int)(start_pixel_y + k))[3] = MTSDF_ASCII(j, k)[3];
                                                }
                                          }
                                    } else {
                                          msdfgen::generateMTSDF(MTSDF, shape, ts);
                                          for (int j = 0; j < 32; j++) {
                                                for (int k = 0; k < 32; k++) {
                                                      bitmap((int)(start_pixel_x + j), (int)(start_pixel_y + k))[0] = MTSDF(j, k)[0];
                                                      bitmap((int)(start_pixel_x + j), (int)(start_pixel_y + k))[1] = MTSDF(j, k)[1];
                                                      bitmap((int)(start_pixel_x + j), (int)(start_pixel_y + k))[2] = MTSDF(j, k)[2];
                                                      bitmap((int)(start_pixel_x + j), (int)(start_pixel_y + k))[3] = MTSDF(j, k)[3];
                                                }
                                          }
                                    }
                              }
                        }

                        for (auto& handle : handles) {
                              handle.wait();
                        }

                        printf("Generate Done");
                        msdfgen::BitmapConstRef<float, 4> ref = bitmap;
                        stbi_write_hdr("D:\\out.hdr", ref.width, ref.height, 4, ref.pixels);
                        printf("Start Uploading Font Altas %d Bytes\n", ref.width * ref.height * 3 * 4);
                        std::string tex_name = std::format("MTSDF Texture for FontAtlas-{}", path);
                        _fontAtlas = _gfx->CreateTexture2D(VK_FORMAT_R32G32B32A32_SFLOAT, ref.width, ref.height, {
                              .pResourceName = tex_name.c_str()
                        });
                        if (!_gfx->SetTexture2D(_fontAtlas, ref.pixels, ref.width * ref.height * 4 * 4)) {
                              const auto str = std::format("PfxContext::LoadFont - Can't Upload Font {}", path);
                              MessageManager::Log(MessageType::Error, str);
                              throw std::runtime_error(str);
                        }
                        _fontAtlasTextureBindlessIndex = _gfx->GetTextureBindlessIndex(_fontAtlas);

                        _fontAtlasSize = VkExtent2D{(uint32_t)ref.width, (uint32_t)ref.height};
                  }
                  msdfgen::destroyFont(font);
            } else {
                  const auto str = std::format("PfxContext::LoadFont - Can't Load Font {}", path);
                  MessageManager::Log(MessageType::Error, str);
                  return false;
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
      _vertexData.emplace_back(center + glm::vec2(ex_size, ex_size), glm::vec2(1, 1));
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

void PfxContext::DrawText(glm::vec2 start, const wchar_t* text, PParamText param, glm::u8vec4 color) {
      std::wstring_view wstr(text);
      if (wstr.empty()) return;

      uint32_t voffset = _vertexData.size();
      uint32_t ioffset = _indexData.size();
      uint32_t indirect_offset = _indirectData.size();

      _vertexData.reserve(voffset + 4 * wstr.size());
      _indexData.reserve(ioffset + 6 * wstr.size());

      float size = param.Size;

      uint32_t dy_voffset = voffset;

      glm::vec2 start_offset = start;
      for (wchar_t i : wstr) {
            FontUV fuv{};
            if (_fontUVs.find(i) == _fontUVs.end()) {
                  fuv = _fontUVs[L'#'];
            } else {
                  fuv = _fontUVs[i];
            }

            if (_fontDOT.contains(i)) {
                  float half_size = size / 2.0f;
                  _vertexData.emplace_back(start_offset + glm::vec2(0, half_size), glm::vec2(fuv.x, fuv.y + fuv.h));
                  _vertexData.emplace_back(start_offset + glm::vec2(half_size, half_size), glm::vec2(fuv.x + fuv.w, fuv.y + fuv.h));
                  _vertexData.emplace_back(start_offset, glm::vec2(fuv.x, fuv.y));
                  _vertexData.emplace_back(start_offset + glm::vec2(half_size, 0), glm::vec2(fuv.x + fuv.w, fuv.y));
                  start_offset.x += half_size + param.Space;
            } else if (std::isalpha(i) || std::isdigit(i) || std::isspace(i) || std::ispunct(i)) {
                  float half_size = size / 2.0f;
                  _vertexData.emplace_back(start_offset + glm::vec2(0, size), glm::vec2(fuv.x, fuv.y + fuv.h));
                  _vertexData.emplace_back(start_offset + glm::vec2(half_size, size), glm::vec2(fuv.x + fuv.w, fuv.y + fuv.h));
                  _vertexData.emplace_back(start_offset, glm::vec2(fuv.x, fuv.y));
                  _vertexData.emplace_back(start_offset + glm::vec2(half_size, 0), glm::vec2(fuv.x + fuv.w, fuv.y));
                  start_offset.x += half_size + param.Space;
            } else {
                  _vertexData.emplace_back(start_offset + glm::vec2(0, size), glm::vec2(fuv.x, fuv.y + fuv.h));
                  _vertexData.emplace_back(start_offset + size, glm::vec2(fuv.x + fuv.w, fuv.y + fuv.h));
                  _vertexData.emplace_back(start_offset, glm::vec2(fuv.x, fuv.y));
                  _vertexData.emplace_back(start_offset + glm::vec2(size, 0), glm::vec2(fuv.x + fuv.w, fuv.y));
                  start_offset.x += size + param.Space;
            }
            _indexData.emplace_back(dy_voffset + 0);
            _indexData.emplace_back(dy_voffset + 1);
            _indexData.emplace_back(dy_voffset + 2);
            _indexData.emplace_back(dy_voffset + 2);
            _indexData.emplace_back(dy_voffset + 1);
            _indexData.emplace_back(dy_voffset + 3);


            dy_voffset += 4;
      }

      _indirectData.emplace_back(
      6 * wstr.size(),
      1,
      ioffset,
      0,
      indirect_offset);

      auto total_size = glm::vec2(start_offset.x, size);
      auto& ref = NewDrawInstanceData();
      ref.Color = color;
      ref.CenterRotate = 0;
      ref.Size = total_size;
      ref.Center = start + total_size / 2.0f;
      ref.PrimitiveType = DrawPrimitveType::Text;
      ref.PrimitiveParameter.Text = param;
      ref.TextureBindlessIndex_or_GradientDataOffset = _fontAtlasTextureBindlessIndex;
}

void PfxContext::EmitDrawCommand(RenderNode* node) {
      if (_vertexData.empty()) return;

      auto current_index = _gfx->GetCurrentFrameIndex();
      for (auto i : _sampledImageReference[current_index]) {
            node->CmdAsSampledTexure(i, GfxEnumKernelType::GRAPHICS);
      }

      node->CmdBindKernel(_kernelDraw);
      node->CmdBindVertexBuffer(_bufferVertex[current_index], 0);
      node->CmdBindVertexBuffer(_bufferInstance[current_index], 1);
      node->CmdBindIndexBuffer(_bufferIndex[current_index], 0);
      node->CmdDrawIndexedIndirect(_bufferIndirect[current_index], 0, _indirectData.size(), sizeof(VkDrawIndexedIndirectCommand));
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
      _canvasSizeStack.emplace_back(_currentCanvasSize);
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
                  current_instance_data.TextureBindlessIndex_or_GradientDataOffset = _gfx->GetTextureBindlessIndex(std::bit_cast<ResourceHandle>(_currentStrock.Texture.ImageHandle));
                  _sampledImageReference[_gfx->GetCurrentFrameIndex()].insert(std::bit_cast<ResourceHandle>(_currentStrock.Texture.ImageHandle));
                  break;
            case StrockFillType::Linear2:
            case StrockFillType::Linear3:
            case StrockFillType::Linear4:
            case StrockFillType::Linear5:
            case StrockFillType::Linear6:
                  current_instance_data.TextureBindlessIndex_or_GradientDataOffset = _gradientData.size();
                  _gradientData.emplace_back(
                  glm::vec2(0, 0),
                  _currentStrock.Linear.DirectionAngle,
                  _currentStrock.Linear.Pos1,
                  _currentStrock.Linear.Pos2,
                  _currentStrock.Linear.Pos3,
                  _currentStrock.Linear.Pos4,
                  _currentStrock.Linear.Pos5,
                  _currentStrock.Linear.Pos6,
                  _currentStrock.Linear.Color1,
                  _currentStrock.Linear.Color2,
                  _currentStrock.Linear.Color3,
                  _currentStrock.Linear.Color4,
                  _currentStrock.Linear.Color5,
                  _currentStrock.Linear.Color6
                  );
                  break;
            case StrockFillType::Radial2:
            case StrockFillType::Radial3:
            case StrockFillType::Radial4:
            case StrockFillType::Radial5:
            case StrockFillType::Radial6:
                  current_instance_data.TextureBindlessIndex_or_GradientDataOffset = _gradientData.size();
                  _gradientData.emplace_back(
                  glm::vec2(_currentStrock.Radial.OffsetX, _currentStrock.Radial.OffsetY),
                  0,
                  _currentStrock.Radial.Pos1,
                  _currentStrock.Radial.Pos2,
                  _currentStrock.Radial.Pos3,
                  _currentStrock.Radial.Pos4,
                  _currentStrock.Radial.Pos5,
                  _currentStrock.Radial.Pos6,
                  _currentStrock.Radial.Color1,
                  _currentStrock.Radial.Color2,
                  _currentStrock.Radial.Color3,
                  _currentStrock.Radial.Color4,
                  _currentStrock.Radial.Color5,
                  _currentStrock.Radial.Color6
                  );
                  break;
      }

      return current_instance_data;
}

void PfxContext::DispatchGenerateCommands() const {
      if (_vertexData.empty()) return;

      auto ptr_vbuf = _gfx->ResourceFetch<Component::Gfx::Buffer>(_bufferVertex[_gfx->GetCurrentFrameIndex()]);
      auto ptr_ibuf = _gfx->ResourceFetch<Component::Gfx::Buffer>(_bufferIndex[_gfx->GetCurrentFrameIndex()]);
      auto ptr_instancebuf = _gfx->ResourceFetch<Component::Gfx::Buffer>(_bufferInstance[_gfx->GetCurrentFrameIndex()]);
      auto ptr_indirectbuf = _gfx->ResourceFetch<Component::Gfx::Buffer>(_bufferIndirect[_gfx->GetCurrentFrameIndex()]);

      auto ptr_gradientbuf = _gfx->ResourceFetch<Component::Gfx::Buffer>(_bufferGradient[_gfx->GetCurrentFrameIndex()]);

      ptr_vbuf->SetData(_vertexData.data(), _vertexData.size() * sizeof(GenDrawVertexData));
      ptr_ibuf->SetData(_indexData.data(), _indexData.size() * sizeof(uint32_t));

      ptr_instancebuf->SetData(_instanceData.data(), _instanceData.size() * sizeof(GenInstanceData));
      ptr_indirectbuf->SetData(_indirectData.data(), _indirectData.size() * sizeof(VkDrawIndexedIndirectCommand));

      if (!_gradientData.empty()) {
            ptr_gradientbuf->SetData(_gradientData.data(), _gradientData.size() * sizeof(GradientData));
            uint64_t address = ptr_gradientbuf->GetBDAAddress();
            _gfx->FillKernelConstant(_kernelDraw, &address, sizeof(address));
      }
}

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

      _fontDOT.insert(L',');
      _fontDOT.insert(L'.');
      _fontDOT.insert(L'_');


      for (int i = 0; i < 3; i++) {
            _bufferVertex[i] = _gfx->CreateBuffer({.DataSize = 8192, .bCpuAccess = true});
            _bufferIndex[i] = _gfx->CreateBuffer({.DataSize = 8192, .bCpuAccess = true});

            _bufferInstance[i] = _gfx->CreateBuffer({.DataSize = 8192, .bCpuAccess = true});
            _bufferIndirect[i] = _gfx->CreateBuffer({.DataSize = 8192, .bCpuAccess = true});

            _bufferGradient[i] = _gfx->CreateBuffer({.DataSize = 8192, .bCpuAccess = true});
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


      const char* path[2] = {"D://DDraw.fs", "D://DDraw.vs"};

      _programDraw = _gfx->CreateProgramFromFile({
            .pResourceName = "PfxContext-Draw-Program",
            .pConfig = draw_config,
            .pSourceCodeFileNames = path,
            .countSourceCodeFileName = 2
      });

      _kernelDraw = _gfx->CreateKernel(_programDraw, {.pResourceName = "PfxContext-Draw-Kernel"});
      if(_kernelDraw.Type != GfxEnumResourceType::Kernel) {
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
      return 1.f/255.f*static_cast<float>(x);
}

bool PfxContext::GenAndLoadFont(const char* path) {

      if (msdfgen::FreetypeHandle* ft = msdfgen::initializeFreetype()) {
            if (msdfgen::FontHandle* font = msdfgen::loadFont(ft, path); font) {

                  const auto font_dataset_path = "f.txt";
                  std::locale locale(std::locale::classic(), new std::codecvt_utf8<wchar_t>);
                  std::wifstream file(font_dataset_path);

                  if(file.is_open()) {

                        file.imbue(locale);
                        std::wstringstream fs{};
                        while(fs << file.rdbuf()) {}
                        file.close();

                        std::wstring font_table = fs.str();

                        msdfgen::Bitmap<float, 3> bitmap(3200, 3200);

                        constexpr auto split = 250;
                        uint32_t threads = font_table.size() / split;

                        std::vector<std::future<void>> handles{};
                        printf("Star Gen %d th", threads);

                        std::vector<msdfgen::Shape> shapes{};

                        double global_size = 99999.0f;
                        double total_bottom_offset = 99999.0f;
                        for(int32_t i = 0; i < font_table.size(); i++) {

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
                              float pixel_uvw = 32.0f  / 3200.0f;
                              float pixel_uvh = 32.0f / 3200.0f;
                              if(_fontDOT.contains(font_table[i])) {
                                    pixel_uvw = 16.0f / 3200.0f;
                                    pixel_uvh = 16.0f / 3200.0f;
                              } else if(std::isalpha(font_table[i]) || std::isdigit(font_table[i]) || std::isspace(font_table[i]) || std::ispunct(font_table[i])) {
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

                              if (dims.x*frame.y < dims.y*frame.x) {
                                    if(frame.y/dims.y >= 0.0001) {
                                          global_size = glm::min(global_size, frame.y/dims.y);
                                          printf("Global %lf, Bound %lf %lf %lf %lf, size %lf ;\n", global_size, bounds.l, bounds.b, bounds.r, bounds.t, frame.y/dims.y);
                                    }

                              } else {
                                    if(frame.x/dims.x >= 0.0001) {
                                          global_size = glm::min(global_size, frame.x/dims.x);
                                          printf("Global %lf, Bound %lf %lf %lf %lf, size %lf ;\n", global_size, bounds.l, bounds.b, bounds.r, bounds.t, frame.x/dims.x);
                                    }
                              }
                              shapes.emplace_back(std::move(shape));
                        }

                        for(int32_t th = 0; th < threads; th++) {

                              uint32_t g_start_i = th * split;
                              uint32_t g_end_i = (th + 1) * split;
                              handles.emplace_back(std::async([&](uint32_t start_i, uint32_t end_i) {

                                    msdfgen::Bitmap<float, 3> MTSDF(32, 32);
                                    msdfgen::Bitmap<float, 3> MTSDF_ASCII(16, 32);
                                    msdfgen::Bitmap<float, 3> MTSDF_Dot(16, 16);
                                    for (uint32_t i = start_i; i < end_i; i++) {
                                          uint32_t x = i % 100;
                                          uint32_t y = i / 100;

                                          uint32_t start_pixel_x = x * 32;
                                          uint32_t start_pixel_y = y * 32;

                                          msdfgen::Vector2 frame(0, 0);
                                          if(_fontDOT.contains(font_table[i])) {
                                                frame = msdfgen::Vector2(10, 10);
                                          } else if(std::isalpha(font_table[i]) || std::isdigit(font_table[i]) || std::isspace(font_table[i]) || std::ispunct(font_table[i])) {
                                                frame = msdfgen::Vector2(16, 32);
                                          }  else {
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

                                          if (dims.x*frame.y < dims.y*frame.x) {
                                               translate.set(.5*(frame.x/frame.y*dims.y-dims.x)-bounds.l, -total_bottom_offset); // -bounds.b
                                                scale = global_size;
                                           } else {
                                               translate.set(-bounds.l, -total_bottom_offset); //5*(frame.y/frame.x*dims.x-dims.y)-bounds.b
                                                scale = global_size;
                                           }
                                          translate -= pxRange.lower/scale;
                                          msdfgen::Range range = pxRange/glm::min(scale.x, scale.y);
                                          msdfgen::SDFTransformation ts(msdfgen::Projection(scale, translate), range);

                                          if(_fontDOT.contains(font_table[i])) {
                                                msdfgen::generateMSDF(MTSDF_Dot, shape, ts);
                                                for (int j = 0; j < 16; j++) {
                                                      for (int k = 0; k < 16; k++) {
                                                            bitmap((int)(start_pixel_x + j), (int)(start_pixel_y + k))[0] = MTSDF_Dot(j, k)[0];
                                                            bitmap((int)(start_pixel_x + j), (int)(start_pixel_y + k))[1] = MTSDF_Dot(j, k)[1];
                                                            bitmap((int)(start_pixel_x + j), (int)(start_pixel_y + k))[2] = MTSDF_Dot(j, k)[2];
                                                      }
                                                }
                                          }
                                          else if(std::isalpha(font_table[i]) || std::isdigit(font_table[i]) || std::isspace(font_table[i]) || std::ispunct(font_table[i])) {
                                                msdfgen::generateMSDF(MTSDF_ASCII, shape, ts);
                                                for (int j = 0; j < 16; j++) {
                                                      for (int k = 0; k < 32; k++) {
                                                            bitmap((int)(start_pixel_x + j), (int)(start_pixel_y + k))[0] = MTSDF_ASCII(j, k)[0];
                                                            bitmap((int)(start_pixel_x + j), (int)(start_pixel_y + k))[1] = MTSDF_ASCII(j, k)[1];
                                                            bitmap((int)(start_pixel_x + j), (int)(start_pixel_y + k))[2] = MTSDF_ASCII(j, k)[2];
                                                      }
                                                }
                                          }  else {
                                                msdfgen::generateMSDF(MTSDF, shape, ts);
                                                for (int j = 0; j < 32; j++) {
                                                      for (int k = 0; k < 32; k++) {
                                                            bitmap((int)(start_pixel_x + j), (int)(start_pixel_y + k))[0] = MTSDF(j, k)[0];
                                                           bitmap((int)(start_pixel_x + j), (int)(start_pixel_y + k))[1] = MTSDF(j, k)[1];
                                                           bitmap((int)(start_pixel_x + j), (int)(start_pixel_y + k))[2] = MTSDF(j, k)[2];
                                                      }
                                                }
                                          }
                                    }
                              }, g_start_i, g_end_i));
                        }

                        {
                              uint32_t left = font_table.size() % split;

                              msdfgen::Bitmap<float, 3> MTSDF(32, 32);
                              msdfgen::Bitmap<float, 3> MTSDF_ASCII(16, 32);
                              msdfgen::Bitmap<float, 3> MTSDF_Dot(16, 16);

                              for (uint32_t i = threads * split; i < threads * split + left; i++) {
                                    uint32_t x = i % 100;
                                    uint32_t y = i / 100;

                                    uint32_t start_pixel_x = x * 32;
                                    uint32_t start_pixel_y = y * 32;

                                    msdfgen::Vector2 frame;
                                    if(_fontDOT.contains(font_table[i])) {
                                          frame = msdfgen::Vector2(10, 10);
                                    } else if(std::isalpha(font_table[i]) || std::isdigit(font_table[i]) || std::isspace(font_table[i]) || std::ispunct(font_table[i])) {
                                          frame = msdfgen::Vector2(16, 32);
                                    }  else {
                                          frame = msdfgen::Vector2(32, 32);
                                    }

                                    msdfgen::Shape& shape = shapes[i];
                                    auto bounds = shape.getBounds();
                                    msdfgen::Range pxRange(4);
                                    frame += 2 * pxRange.lower;
                                    msdfgen::Vector2 translate;
                                    msdfgen::Vector2 scale;
                                    msdfgen::Vector2 dims(bounds.r - bounds.l, bounds.t - bounds.b);

                                    if (dims.x*frame.y < dims.y*frame.x) {
                                         translate.set(.5*(frame.x/frame.y*dims.y-dims.x)-bounds.l, -bounds.b);
                                         scale = global_size;//frame.y/dims.y;
                                    } else {
                                        translate.set(-bounds.l, .5*(frame.y/frame.x*dims.x-dims.y)-bounds.b);
                                        scale  = global_size;//frame.x/dims.x;
                                    }

                                    translate -= pxRange.lower/scale;
                                    msdfgen::Range range = pxRange/glm::min(scale.x, scale.y);
                                    msdfgen::SDFTransformation ts(msdfgen::Projection(scale, translate), range);

                                    if(_fontDOT.contains(font_table[i])) {
                                          msdfgen::generateMSDF(MTSDF_Dot, shape, ts);
                                          for (int j = 0; j < 16; j++) {
                                                for (int k = 0; k < 16; k++) {
                                                      bitmap((int)(start_pixel_x + j), (int)(start_pixel_y + k))[0] = MTSDF_Dot(j, k)[0];
                                                      bitmap((int)(start_pixel_x + j), (int)(start_pixel_y + k))[1] = MTSDF_Dot(j, k)[1];
                                                      bitmap((int)(start_pixel_x + j), (int)(start_pixel_y + k))[2] = MTSDF_Dot(j, k)[2];
                                                }
                                          }
                                    }
                                    else if(std::isalpha(font_table[i]) || std::isdigit(font_table[i]) || std::isspace(font_table[i]) || std::ispunct(font_table[i])) {
                                          msdfgen::generateMSDF(MTSDF_ASCII, shape, ts);
                                          for (int j = 0; j < 16; j++) {
                                                for (int k = 0; k < 32; k++) {
                                                      bitmap((int)(start_pixel_x + j), (int)(start_pixel_y + k))[0] = MTSDF_ASCII(j, k)[0];
                                                      bitmap((int)(start_pixel_x + j), (int)(start_pixel_y + k))[1] = MTSDF_ASCII(j, k)[1];
                                                      bitmap((int)(start_pixel_x + j), (int)(start_pixel_y + k))[2] = MTSDF_ASCII(j, k)[2];
                                                }
                                          }
                                    }  else {
                                          msdfgen::generateMSDF(MTSDF, shape, ts);
                                          for (int j = 0; j < 32; j++) {
                                                for (int k = 0; k < 32; k++) {
                                                      bitmap((int)(start_pixel_x + j), (int)(start_pixel_y + k))[0] = MTSDF(j, k)[0];
                                                     bitmap((int)(start_pixel_x + j), (int)(start_pixel_y + k))[1] = MTSDF(j, k)[1];
                                                     bitmap((int)(start_pixel_x + j), (int)(start_pixel_y + k))[2] = MTSDF(j, k)[2];
                                                }
                                          }
                                    }

                              }
                        }

                        for(auto& handle : handles) {
                              handle.wait();
                        }

                        printf("Generate Done");
                        msdfgen::BitmapConstRef<float, 3> ref = bitmap;
                        stbi_write_hdr("D:\\out.hdr", ref.width, ref.height, 3, ref.pixels);

                        printf("Start Uploading Font Altas %d Bytes\n", ref.width * ref.height * 3 * 4);
                        _fontAtlas = _gfx->CreateTexture2D(VK_FORMAT_R32G32B32_SFLOAT, ref.width, ref.height);
                        if(!_gfx->SetTexture2D(_fontAtlas, ref.pixels, ref.width * ref.height * 4 * 3)) {
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

void PfxContext::DrawText(glm::vec2 start, const wchar_t* text, PParamText param, glm::u8vec4 color) {
      std::wstring_view wstr(text);
      if(wstr.empty()) return;

      uint32_t voffset = _vertexData.size();
      uint32_t ioffset = _indexData.size();
      uint32_t indirect_offset = _indirectData.size();

      _vertexData.reserve(voffset + 4 * wstr.size());
      _indexData.reserve(ioffset + 6 * wstr.size());

      float size = param.Size;

      uint32_t dy_voffset = voffset;

      glm::vec2 start_offset = start;
      for(wchar_t i : wstr) {
            FontUV fuv{};
            if(_fontUVs.find(i) == _fontUVs.end()) {
                  fuv = _fontUVs[L'#'];
            } else {
                  fuv = _fontUVs[i];
            }

            if(_fontDOT.contains(i)) {
                  float half_size = size / 2.0f;
                  _vertexData.emplace_back(start_offset + glm::vec2(0, half_size), glm::vec2(fuv.x, fuv.y + fuv.h));
                  _vertexData.emplace_back(start_offset + glm::vec2(half_size, half_size), glm::vec2(fuv.x + fuv.w, fuv.y + fuv.h));
                  _vertexData.emplace_back(start_offset, glm::vec2(fuv.x, fuv.y));
                  _vertexData.emplace_back(start_offset + glm::vec2(half_size, 0), glm::vec2(fuv.x + fuv.w, fuv.y));
                  start_offset.x += half_size + param.Space;
            } else if(std::isalpha(i) || std::isdigit(i) || std::isspace(i) || std::ispunct(i)) {
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
                  current_instance_data.TextureBindlessIndex_or_GradientDataOffset = _gfx->GetTextureBindlessIndex(std::bit_cast<ResourceHandle>(_currentStrock.Texture.ImageHandle));
                  _sampledImageReference[_gfx->GetCurrentFrameIndex()].insert(std::bit_cast<ResourceHandle>(_currentStrock.Texture.ImageHandle));
                  break;
            case StrockFillType::Linear2:
            case StrockFillType::Linear3:
            case StrockFillType::Linear4:
            case StrockFillType::Linear5:
            case StrockFillType::Linear6:
            case StrockFillType::Linear7:
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
            case StrockFillType::Radial7:
                  current_instance_data.TextureBindlessIndex_or_GradientDataOffset = _gradientData.size();
                  _gradientData.emplace_back(
                  glm::vec2(0, 0),
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

      if(!_gradientData.empty()) {
            ptr_gradientbuf->SetData(_gradientData.data(), _gradientData.size() * sizeof(GradientData));
            uint64_t address = ptr_gradientbuf->GetBDAAddress();
            _gfx->FillKernelConstant(_kernelDraw, &address, sizeof(address));
      }
}

//
// Created by starr on 2024/6/20.
//
#include "LoFiGfx.h"

#include <future>
#include <iostream>
#include <thread>

#include "Context.h"
#include "entt/entt.hpp"

void GStart()
{
      std::string_view hlsl_str = R"(
            #set rt = r8g8b8a8_unorm
            //#set ds = d32_sfloat_s8_uint

            #set color_blend = false

            //#set vs_location = 0 0 r32g32b32a32_sfloat 0
            //#set vs_location = 0 1 r32g32b32a32_sfloat 16

            //#set vs_binding = 0 32 vertex

            //#set depth_write = false
            //#set depth_test = false

            #set topology   = triangle_list
            #set polygon_mode = fill
            #set cull_mode   = back

            //#set front_face  = clockwise
            //#set depth_bias = 0 0 0

            //[[vk::binding(2, 0)]] Texture2D textures[];
            //SamplerState samplerColorMap;

            //struct TypeBufferInput {
            //    float4 pos22;
            //    float4 pos33;
            //    float4 pos55;
            //}; [[vk::binding(0, 0)]] StructuredBuffer<TypeBufferInput> input[];

            //[[vk::push_constant]] struct PushConsts {
	      //      float4 color;
	      //      float4 position;
            //} pushConsts;

            //struct VSInput {
               //[[vk::location(0)]] float4 pos : POSITION;
              // [[vk::location(1)]] float4 color : COLOR;
            //};

            struct VSOutPut {
                  float2 pos  : POSITION;
                  //float4 color : COLOR;
            };

            static float2 positions[3] = {
                float2(0.0f, -0.3f),
                float2(0.5f, 0.5f),
                float2(-0.5f, 0.8f)
            };

            VSOutPut VSMain(uint VertexIndex : SV_VertexID) {
	            VSOutPut output;
	            output.pos = positions[VertexIndex];
	            //output.color = input.color;
	            return output;
            }

            float4 PSMain(VSOutPut input) : SV_Target {
                  float4 color =  float4(1.0f, 1.0f, 1.0f, 1.0f);
                  return color;
            }
      )";

      //4 point from [-1, -1] to [1, 1]

      //triangle
      std::array triangle_vt = {
            0.0f, 0.5f,  1.0f, 0.0f, 0.0f,
            0.5f, -0.5f,   0.0f, 1.0f, 0.0f,
            -0.5f, -0.5f,   0.0f, 0.0f, 1.0f,
      };

      std::array<uint32_t, 3> triangle_id = {0, 1, 2,};

      //square
      std::array square_vt = {
            -0.5f, 0.5f,  1.0f, 0.0f, 0.0f,
            0.5f, 0.5f,   0.0f, 1.0f, 0.0f,
            0.5f, -0.5f,   0.0f, 0.0f, 1.0f,
            -0.5f, -0.5f,   1.0f, 1.0f, 1.0f,
      };

      std::array<uint32_t, 6> square_id = {0, 1, 2, 0, 2, 3};

      std::array rect_vt = {
            -0.8f, 0.5f,  1.0f, 0.0f, 0.0f,
            0.8f, 0.5f,   0.0f, 1.0f, 0.0f,
            0.8f, -0.5f,   0.0f, 0.0f, 1.0f,
            -0.8f, -0.5f,   1.0f, 1.0f, 1.0f,
      };

      std::array<uint32_t, 6> rect_id = {0, 1, 2, 0, 2, 3};

      auto ctx = std::make_unique<LoFi::Context>();

      ctx->Init();
      const char* vs = R"(
                  #set topology   = triangle_list
                  #set polygon_mode = fill
                  #set cull_mode   = none

                  #set vs_location = 0 0 r32g32_sfloat 0
                  layout(location = 0) in vec2 pos;

                  #set vs_location = 0 1 r32g32b32_sfloat 12
                  layout(location = 1) in vec3 color;

                  #set vs_binding = 0 20 vertex

                  layout(location = 0) out vec3 out_color;

                  #extension GL_EXT_nonuniform_qualifier : enable
                  #define BindlessStorageBinding 0
                  #define BindlessSamplerBinding 1

                  #define GetLayoutVariableName(Name) _bindless##Name##Register

                  #define RegisterStruct1(Name, Vars) \
                  layout(set = 0, binding = BindlessStorageBinding) buffer Name Vars GetLayoutVariableName(Name)[];

                  #define RegisterStruct2(Name, Vars, Name2, Vars2) \
                  layout(set = 0, binding = BindlessStorageBinding) buffer Name Vars GetLayoutVariableName(Name)[]; \
                  layout(set = 0, binding = BindlessStorageBinding) buffer Name2 Vars2 GetLayoutVariableName(Name2)[];

                  #define RegisterStruct3(Name, Vars, Name2, Vars2, Name3, Vars3) \
                  layout(set = 0, binding = BindlessStorageBinding) buffer Name Vars GetLayoutVariableName(Name)[]; \
                  layout(set = 0, binding = BindlessStorageBinding) buffer Name2 Vars2 GetLayoutVariableName(Name2)[]; \
                  layout(set = 0, binding = BindlessStorageBinding) buffer Name3 Vars3 GetLayoutVariableName(Name3)[];

                  #define RegisterStruct4(Name, Vars, Name2, Vars2, Name3, Vars3, Name4, Vars4) \
                  layout(set = 0, binding = BindlessStorageBinding) buffer Name Vars GetLayoutVariableName(Name)[]; \
                  layout(set = 0, binding = BindlessStorageBinding) buffer Name2 Vars2 GetLayoutVariableName(Name2)[]; \
                  layout(set = 0, binding = BindlessStorageBinding) buffer Name3 Vars3 GetLayoutVariableName(Name3)[]; \
                  layout(set = 0, binding = BindlessStorageBinding) buffer Name4 Vars4 GetLayoutVariableName(Name4)[];

                  #define RegisterStruct5(Name, Vars, Name2, Vars2, Name3, Vars3, Name4, Vars4, Name5, Vars5) \
                  layout(set = 0, binding = BindlessStorageBinding) buffer Name Vars GetLayoutVariableName(Name)[]; \
                  layout(set = 0, binding = BindlessStorageBinding) buffer Name2 Vars2 GetLayoutVariableName(Name2)[]; \
                  layout(set = 0, binding = BindlessStorageBinding) buffer Name3 Vars3 GetLayoutVariableName(Name3)[]; \
                  layout(set = 0, binding = BindlessStorageBinding) buffer Name4 Vars4 GetLayoutVariableName(Name4)[]; \
                  layout(set = 0, binding = BindlessStorageBinding) buffer Name5 Vars5 GetLayoutVariableName(Name5)[];

                  RegisterStruct2(
                        Info, {
                              vec4 pos;
                              vec4 color;
                        },
                        Camera, {
                              mat4 view;
                              mat4 proj;
                        }
                  )

                  void VSMain() {
                        gl_Position = vec4(pos, 0.0f, 1.0f);
                        out_color = color;
                  }
      )";

      const char* ps = R"(
            #set rt = r8g8b8a8_unorm
            #set color_blend = false

            layout(location = 0) in vec3 color;
            layout(location = 0) out vec4 outColor;

            void FSMain() {
                  outColor = vec4(color, 1.0f);
            }
      )";

      auto win1 = ctx->CreateWindow("window1", 800, 600);
      auto win2 = ctx->CreateWindow("window2", 400, 400);
      auto win3 = ctx->CreateWindow("window3", 800, 600);

      auto rt1 = ctx->CreateTexture2D(VK_FORMAT_R8G8B8A8_UNORM, 800, 600);
      auto rt2 = ctx->CreateTexture2D(VK_FORMAT_R8G8B8A8_UNORM, 400, 400);
      auto rt3 = ctx->CreateTexture2D(VK_FORMAT_R8G8B8A8_UNORM, 800, 600);

      ctx->MapRenderTargetToWindow(rt1, win1);
      ctx->MapRenderTargetToWindow(rt2, win2);
      ctx->MapRenderTargetToWindow(rt3, win3);

      auto triangle_vert = ctx->CreateBuffer(triangle_vt.data(), triangle_vt.size() * sizeof(float));
      auto triangle_index = ctx->CreateBuffer(triangle_id.data(), triangle_id.size() * sizeof(uint32_t));

      auto square_vert = ctx->CreateBuffer(square_vt.data(), square_vt.size() * sizeof(float));
      auto square_index = ctx->CreateBuffer(square_id.data(), square_id.size() * sizeof(uint32_t));

      auto rect_vert = ctx->CreateBuffer(rect_vt.data(), rect_vt.size() * sizeof(float));
      auto rect_index = ctx->CreateBuffer(rect_id.data(), rect_id.size() * sizeof(uint32_t));

      auto program = ctx->CreateProgram({vs, ps});
      auto kernel = ctx->CreateGraphicKernel(program);

      std::atomic<bool> should_close = false;
      auto func = std::async(std::launch::async, [&] {
            while (!should_close) {
                  ctx->BeginFrame();

                  ctx->CmdBindRenderTargetBeforeRenderPass(rt1);
                  ctx->CmdBeginRenderPass();
                  ctx->CmdBindGraphicKernelToRenderPass(kernel);
                  ctx->CmdBindVertexBuffer(triangle_vert);
                  ctx->CmdDrawIndex(triangle_index);
                  ctx->CmdEndRenderPass();

                  ctx->CmdBindRenderTargetBeforeRenderPass(rt2);
                  ctx->CmdBeginRenderPass();
                  ctx->CmdBindGraphicKernelToRenderPass(kernel);
                  ctx->CmdBindVertexBuffer(square_vert);
                  ctx->CmdDrawIndex(square_index);
                  ctx->CmdEndRenderPass();

                  ctx->CmdBindRenderTargetBeforeRenderPass(rt3);
                  ctx->CmdBeginRenderPass();
                  ctx->CmdBindGraphicKernelToRenderPass(kernel);
                  ctx->CmdBindVertexBuffer(rect_vert);
                  ctx->CmdDrawIndex(rect_index);
                  ctx->CmdEndRenderPass();


                  ctx->EndFrame();
            }
      });

      while (ctx->PollEvent()) {}


      should_close = true;

      func.wait();

      printf("你好你好");
}

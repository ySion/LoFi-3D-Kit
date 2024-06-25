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
            #set rt = r8g8b8a8_srgb r8g8b8a8_unorm
            #set ds = d32_sfloat_s8_uint

            #set color_blend = false
            #set color_blend = false

            #set vs_location = 0 0 r32g32b32a32_sfloat 0
            #set vs_location = 1 0 r32g32b32_sfloat 16

            #set vs_binding = 0 32 vertex

            #set depth_write = true
            #set depth_test = default




            //#set topology   = triangle_list
            //#set polygon_mode = fill
            //#set cull_mode   = back
            //#set front_face  = clockwise
            //#set depth_bias = 0 0 0





            [[vk::binding(2, 0)]] Texture2D textures[];
            SamplerState samplerColorMap;

            struct TypeBufferInput {
                float4 pos22;
                float4 pos33;
                float4 pos55;
            }; [[vk::binding(0, 0)]] StructuredBuffer<TypeBufferInput> input[];

            [[vk::push_constant]] struct PushConsts {
	            float4 color;
	            float4 position;
            } pushConsts;

            struct VSInput {
              [[vk::location(0)]] float4 pos  : POSITION;
              [[vk::location(1)]] float3 norm : NORMAL;
            };

            struct VSOutPut {
                  float4 pos  : POSITION;
                  float3 norm : NORMAL;
            };

            struct PSOutPut{
                  float4 color1 : SV_Target0;
                  float4 color2 : SV_Target1;
            };

            VSOutPut VSMain(VSInput input) {
	            VSOutPut output;
	            output.pos = input.pos;
	            output.norm = input.norm;
	            return output;
            }

            PSOutPut PSMain(VSOutPut input) {
                  PSOutPut output;
                  output.color1 = input.pos;
                  output.color2 = float4(input.norm, 1.0f);
                  return output;
            }
      )";

      //4 point from [-1, -1] to [1, 1]
      std::array cube = {
            -1.0f, -1.0f,
             -1.0f, 1.0f,
             1.0f, 1.0f,
             1.0f, -1.0f
       };

      auto ctx = std::make_unique<LoFi::Context>();


      ctx->Init();
      ctx->CreateWindow("hello world", 800, 600);
     // ctx->CreateWindow("hello world1", 800, 600);
      //ctx->CreateWindow("hello world2", 800, 600);

      auto renderTexture = ctx->CreateTexture2D(VK_FORMAT_R8G8B8A8_UNORM, 800, 600);
      auto ds_texture = ctx->CreateTexture2D(VK_FORMAT_D32_SFLOAT_S8_UINT, 800, 600);

      //auto vertex_buffer = ctx->CreateBuffer(4 * 512);
      //auto vertex_buffer2 = ctx->CreateBuffer(4 * 512, true);
      //auto vertex_buffer3 = ctx->CreateBuffer(cube.data(), cube.size() * sizeof(float), true);
      //auto vertex_buffer4 = ctx->CreateBuffer(cube.data(), cube.size() * sizeof(float));

      //ctx->DestroyBuffer(vertex_buffer3);
      //ctx->DestroyTexture(renderTexture2);

      auto program = ctx->CreateProgram(hlsl_str);
      auto kernel = ctx->CreateGraphicKernel(program);

      std::atomic<bool> should_close = false;

      auto func = std::async(std::launch::async, [&] {
            while (!should_close) {
                  ctx->BeginFrame();

                  ctx->BindRenderTargetToRenderPass(renderTexture);
                  ctx->BindDepthStencilTargetToRenderPass(ds_texture);
                  ctx->BeginRenderPass();

                  ctx->EndRenderPass();
                  ctx->EndFrame();
            }
      });

      while (ctx->PollEvent()) {

      }
      should_close = true;

      func.wait();

      printf("End");
}

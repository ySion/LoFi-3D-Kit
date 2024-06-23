//
// Created by starr on 2024/6/20.
//
#include "LoFiGfx.h"
#include <iostream>
#include "Context.h"
#include "entt/entt.hpp"

void GStart()
{
      auto ctx = std::make_unique<LoFi::Context>();

      //4 point from [-1, -1] to [1, 1]
      std::array cube = {
           -1.0f, -1.0f,
            -1.0f, 1.0f,
            1.0f, 1.0f,
            1.0f, -1.0f
      };

      ctx->Init();
      ctx->CreateWindow("hello world", 800, 600);
      ctx->CreateWindow("hello world2", 800, 600);

      auto renderTexture = ctx->CreateTexture2D(VK_FORMAT_R8G8B8A8_UNORM, 800, 600);
      auto renderTexture2 = ctx->CreateTexture2D(VK_FORMAT_D32_SFLOAT_S8_UINT, 800, 600);
      auto vertex_buffer = ctx->CreateBuffer(4 * 512);
      auto vertex_buffer2 = ctx->CreateBuffer(4 * 512, true);
      auto vertex_buffer3 = ctx->CreateBuffer(cube.data(), cube.size() * sizeof(float), true);
      auto vertex_buffer4 = ctx->CreateBuffer(cube.data(), cube.size() * sizeof(float));


      std::string_view hlsl_str = R"(

            struct TypeBufferInput {
                float4 pos22;
                float4 pos33;
                float4 pos55;
            };

            StructuredBuffer<TypeBufferInput> input[];

            Texture2D textures[];
            SamplerState samplerColorMap;

            struct PushConsts {
	            float4 color;
	            float4 position;
            };
            [[vk::push_constant]] PushConsts pushConsts;

            struct FromVS {
                  float4 pos  : POSITION;
                  float3 norm : NORMAL;
            };

            float4 PSMain(FromVS input) : SV_Target {
                return float4(input.norm, 1.0f);
            }
      )";

      auto someTest = ctx->CreateProgram(hlsl_str.data());

      while (ctx->PollEvent())
      {
            ctx->BeginFrame();

            ctx->EndFrame();
      }

      printf("End");
}

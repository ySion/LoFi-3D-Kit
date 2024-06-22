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

      auto renderTexture = ctx->CreateRenderTexture(800, 600);
      auto renderTexture2 = ctx->CreateDepthStencil(800, 600);
      auto vertex_buffer = ctx->CreateVertexBuffer(4 * 512);
      auto vertex_buffer2 = ctx->CreateVertexBuffer(4 * 512, true);
      auto vertex_buffer3 = ctx->CreateVertexBuffer(cube.data(), cube.size() * sizeof(float), true);
      auto vertex_buffer4 = ctx->CreateVertexBuffer(cube.data(), cube.size() * sizeof(float));


      while (ctx->PollEvent())
      {
            ctx->BeginFrame();

            ctx->EndFrame();
      }

      printf("End");
}

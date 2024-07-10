//
// Created by starr on 2024/6/20.
//
#include "LoFiGfx.h"

#include <future>

#include "GfxContext.h"
#include "PfxContext.h"
#include "entt/entt.hpp"
#include "SDL3/SDL.h"

void GStart() {
      const auto vs = R"(
            layout(location = 0) in vec3 pos;
            layout(location = 1) in vec3 color;

            layout(location = 0) out vec3 out_pos;

            void VSMain() {
                  gl_Position = vec4(pos, 1.0f);
                  out_pos = pos;
            }
      )";

      const auto ps = R"(
            #set rt = r8g8b8a8_unorm
            #set ds = d32_sfloat

            #set color_blend = false

            layout(location = 0) in vec3 pos;
            layout(location = 0) out vec4 outColor;

            PickWTextures(rgba32f, 1D)

            SAMPLED image1;

            RBUFFER Info {
                  float scale;
            }

            void FSMain() {
                  uint image_hanlde = GetTextureHandle(image1);
                  vec3 f = texture(GetSampled2D(image_hanlde), vec2(pos.x,  1.0-pos.y)).rgb;
                  outColor = vec4(f, 1.0f) * (sin(GetBuffer(Info).scale) * 0.5 + 0.5);
            }
      )";

      const auto font_vs = R"(
            layout(location = 0) in vec2 pos;
            layout(location = 1) in vec2 uv;

            layout(location = 0) out vec2 out_pos;
            layout(location = 1) out vec2 out_uv;

            void VSMain() {
                  gl_Position = vec4(pos, 0.0f, 1.0f);
                  out_pos = pos;
                  out_uv = uv;
            }
      )";

      const auto font_ps = R"(
            #set rt = r8g8b8a8_unorm
            #set ds = d32_sfloat

            #set color_blend = false

            layout(location = 0) in vec2 pos;
            layout(location = 1) in vec2 uv;

            layout(location = 0) out vec4 outColor;

            SAMPLED font_atlas;

            float median(float r, float g, float b) {
                  return max(min(r, g), min(max(r, g), b));
            }

            void FSMain() {
                  uint image_hanlde = GetTextureHandle(font_atlas);
                  vec3 msd = texture(GetSampled2D(image_hanlde), vec2(uv.x,uv.y)).rgb;

                  vec3 msd2 = texture(GetSampled2D(uint(uv.x)), vec2(uv.x,uv.y)).rgb;

                  float sd = median(msd.r, msd.g, msd.b);
                  float screenPxDistance = 200*(sd - 0.5);
                  float opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
                  outColor = mix(vec4(0.0f,0.0f,0.0f,1.0f), vec4(1.0f,1.0f,1.0f,1.0f), opacity);
                  //outColor = vec4(1.0f,1.0f,1.0f, 1.0f);
            }
      )";

      //triangle
      std::array triangle_vt = {
            0.0f, 0.6f, 0.5f, 1.0f, 0.0f, 0.0f,
            0.6f, -0.6f, 0.5f, 0.0f, 1.0f, 0.0f,
            -0.6f, -0.6f, 0.5f, 0.0f, 0.0f, 1.0f,
      };
      std::array triangle_id = {0, 1, 2};

      //square
      std::array square_vt = {
            -0.9f, 0.9f, 0.0f, 1.0f, 1.0f, 0.0f,
            0.9f, 0.9f, 0.0f, 0.0f, 1.0f, 1.0f,
            0.9f, -0.9f, 0.0f, 1.0f, 0.0f, 1.0f,
            -0.9f, -0.9f, 0.0f, 1.0f, 1.0f, 1.0f,
      };

      std::array square_vt2 = {
            -1.0f, 1.0f , 0.0f, 1.0f,
            1.0f, 1.0f, 1.0f, 1.0f,
            1.0f, -1.0f, 1.0f, 0.0f,
            -1.0f, -1.0f, 0.0f, 0.0f,
      };

      std::array square_id = {0, 1, 2, 0, 2, 3};

      std::vector<float> noise_image(512 * 512 * 4);
      for(int i = 0; i < 512; i++) {
            for(int j = 0; j < 512; j++) {
                  noise_image[i*512*4 + j*4 + 0] = sin(i / 10.0f) * 0.5f + 0.5f;
                  noise_image[i*512*4 + j*4 + 1] = cos(j / 10.0f) * 0.5f + 0.5f;
                  noise_image[i*512*4 + j*4 + 2] = 0.5f;
                  noise_image[i*512*4 + j*4 + 3] = 1.0f;
            }
      }

      auto ctx = std::make_unique<LoFi::GfxContext>();
      ctx->Init();

      const auto noise = ctx->CreateTexture2D(noise_image, VK_FORMAT_R32G32B32A32_SFLOAT, 512, 512);

      const auto triangle_vert = ctx->CreateBuffer(triangle_vt);
      const auto triangle_index = ctx->CreateBuffer(triangle_id);

      const auto square_vert = ctx->CreateBuffer(square_vt);
      const auto square_index = ctx->CreateBuffer(square_id);



      //CreateWindows
      //const auto win1 = ctx->CreateWindow("Triangle", 960, 540);
      //const auto win2 = ctx->CreateWindow("Rectangle", 400, 400);
      //const auto win3 = ctx->CreateWindow("Merge", 800, 600);
      const auto win4 = ctx->CreateWindow("Font", 600, 600);

      const auto rt1 = ctx->CreateTexture2D(VK_FORMAT_R8G8B8A8_UNORM, 960, 540);
      const auto rt2 = ctx->CreateTexture2D(VK_FORMAT_R8G8B8A8_UNORM, 400, 400);
      const auto rt3 = ctx->CreateTexture2D(VK_FORMAT_R8G8B8A8_UNORM, 800, 600);
      const auto rt0 = ctx->CreateTexture2D(VK_FORMAT_R8G8B8A8_UNORM, 600, 600);

      const auto ds = ctx->CreateTexture2D(VK_FORMAT_D32_SFLOAT, 800, 600);

      //ctx->MapRenderTargetToWindow(rt1, win1);
      //ctx->MapRenderTargetToWindow(rt2, win2);
      //ctx->MapRenderTargetToWindow(rt3, win3);
      ctx->MapRenderTargetToWindow(rt0, win4);

      const auto program = ctx->CreateProgram({vs, ps});
      const auto kernel = ctx->CreateKernel(program);
      const auto kernel_instance = ctx->CreateKernelInstance(kernel);
      const auto kernel_instance2 = ctx->CreateKernelInstance(kernel);

      const auto info_param = ctx->CreateFrameResource(sizeof(float));

      ctx->BindKernelResource(kernel_instance, "Info", info_param);
      ctx->BindKernelResource(kernel_instance2, "Info", info_param);

      ctx->BindKernelResource(kernel_instance, "image1", noise);
      ctx->BindKernelResource(kernel_instance2, "image1", rt0);

      const auto font_program = ctx->CreateProgram({font_vs, font_ps});
      const auto font_kernel = ctx->CreateKernel(font_program);
      const auto font_kernel_instance = ctx->CreateKernelInstance(font_kernel);


      std::unique_ptr<LoFi::PfxContext> pfx = std::make_unique<LoFi::PfxContext>();
      if(!pfx->LoadFont("D:/llt.ttf")) {
            printf("Start Failed");
            return;
      }

      auto font_altas = pfx->GetAtlas();
      //auto myfront_mesh = pfx->GenText(L'中', 32);
      //const auto square_vert2 = ctx->CreateBuffer(square_vt2);

      // if(myfront_mesh == entt::null) {
      //       printf("GenText Failed");
      //       return;
      // }

      ctx->BindKernelResource(font_kernel_instance, "font_atlas", font_altas);

      auto render_node_2d = pfx->CreateRenderNode();

      pfx->CmdReset(render_node_2d);
      pfx->CmdSetVirtualCanvasSize(render_node_2d, 100, 100);
      pfx->CmdDrawRect(render_node_2d, 50, 40, 40, 50, 1.0f, 0.0f, 0.0f, 0.5f);
      pfx->CmdDrawRect(render_node_2d, 10, 10, 60, 50, 0.0f, 0.0f, 1.0f, 0.5f);
      pfx->CmdGenerate(render_node_2d);



      std::atomic<bool> should_close = false;
      auto func = std::async(std::launch::async, [&] {
            while (!should_close) {
                  float time = (float)((double)SDL_GetTicks() / 1000.0);

                  ctx->SetFrameResourceData(info_param, &time, sizeof(float));

                  ctx->BeginFrame();

                  //Pass 0
                  ctx->CmdBeginPass({{rt0}});
                  pfx->CmdEmitCommand(render_node_2d);
                  ctx->CmdEndPass();


                  //Pass 1
                  ctx->CmdBeginPass({{rt1}});
                  ctx->CmdBindKernel(kernel_instance);
                  ctx->CmdBindVertexBuffer(triangle_vert);
                  ctx->CmdDrawIndex(triangle_index);
                  ctx->CmdEndPass();

                  //Pass 2
                  ctx->CmdBeginPass({{rt2}});
                  ctx->CmdBindKernel(kernel_instance);
                  ctx->CmdBindVertexBuffer(square_vert);
                  ctx->CmdDrawIndex(square_index);
                  ctx->CmdEndPass();

                  //Pass 3
                  ctx->CmdBeginPass({{rt3}, {ds}});
                  ctx->CmdBindKernel(kernel_instance2);
                  ctx->CmdBindVertexBuffer(square_vert);
                  ctx->CmdDrawIndex(square_index);
                  ctx->CmdBindVertexBuffer(triangle_vert);
                  ctx->CmdDrawIndex(triangle_index);
                  ctx->CmdEndPass();

                  ctx->EndFrame();
            }
      });

      while (ctx->PollEvent()) {}
      should_close = true;
      func.wait();

      printf("结束");
}

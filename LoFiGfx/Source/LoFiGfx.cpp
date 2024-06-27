//
// Created by starr on 2024/6/20.
//
#include "LoFiGfx.h"

#include <future>
#include <iostream>
#include <thread>

#include "Context.h"
#include "entt/entt.hpp"

void GStart() {
      const char* vs = R"(
                  #set topology   = triangle_list
                  #set polygon_mode = fill
                  #set cull_mode   = none
                  #set depth_test  = default
                  #set depth_write  = true

                  #set vs_location = 0 0 r32g32b32_sfloat 0
                  layout(location = 0) in vec2 pos;

                  #set vs_location = 0 1 r32g32b32_sfloat 12
                  layout(location = 1) in vec3 color;

                  #set vs_binding = 0 24 vertex

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
            #set ds = d32_sfloat
            #set color_blend = false

            layout(location = 0) in vec3 color;
            layout(location = 0) out vec4 outColor;

            void FSMain() {
                  outColor = vec4(color, 1.0f);
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
            -0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 0.0f,
            0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f,
            0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f,
            -0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 1.0f,
      };

      std::array square_id = {0, 1, 2, 0, 2, 3};

      auto ctx = std::make_unique<LoFi::Context>();
      ctx->Init();

      const auto win1 = ctx->CreateWindow("Triangle", 800, 600);
      const auto win2 = ctx->CreateWindow("Rectangle", 400, 400);
      const auto win3 = ctx->CreateWindow("Depth", 800, 600);

      const auto rt1 = ctx->CreateTexture2D(VK_FORMAT_R8G8B8A8_UNORM, 800, 600);
      const auto rt2 = ctx->CreateTexture2D(VK_FORMAT_R8G8B8A8_UNORM, 400, 400);
      const auto rt3 = ctx->CreateTexture2D(VK_FORMAT_R8G8B8A8_UNORM, 800, 600);

      const auto ds = ctx->CreateTexture2D(VK_FORMAT_D32_SFLOAT, 800, 600);

      ctx->MapRenderTargetToWindow(rt1, win1);
      ctx->MapRenderTargetToWindow(rt2, win2);
      ctx->MapRenderTargetToWindow(rt3, win3);

      const auto triangle_vert = ctx->CreateBuffer(triangle_vt);
      const auto triangle_index = ctx->CreateBuffer(triangle_id);

      const auto square_vert = ctx->CreateBuffer(square_vt);
      const auto square_index = ctx->CreateBuffer(square_id);

      const auto program = ctx->CreateProgram({vs, ps});
      const auto kernel = ctx->CreateGraphicKernel(program);

      std::atomic<bool> should_close = false;
      auto func = std::async(std::launch::async, [&] {
            while (!should_close) {
                  ctx->BeginFrame();

                  //Pass 1
                  ctx->CmdBeginRenderPass({ {rt1} });
                  ctx->CmdBindGraphicKernelToRenderPass(kernel);
                  ctx->CmdBindVertexBuffer(triangle_vert);
                  ctx->CmdDrawIndex(triangle_index);
                  ctx->CmdEndRenderPass();

                  //Pass 2
                  ctx->CmdBeginRenderPass({{rt2}});
                  ctx->CmdBindGraphicKernelToRenderPass(kernel);
                  ctx->CmdBindVertexBuffer(square_vert);
                  ctx->CmdDrawIndex(square_index);
                  ctx->CmdEndRenderPass();

                  //Pass 3
                  ctx->CmdBeginRenderPass({{rt3},{ds}});
                  ctx->CmdBindGraphicKernelToRenderPass(kernel);
                  ctx->CmdBindVertexBuffer(triangle_vert);
                  ctx->CmdDrawIndex(triangle_index);
                  ctx->CmdBindVertexBuffer(square_vert);
                  ctx->CmdDrawIndex(square_index);
                  ctx->CmdEndRenderPass();

                  ctx->EndFrame();
            }
      });

      while (ctx->PollEvent()) {}
      should_close = true;
      func.wait();

      printf("End");
}

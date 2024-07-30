//
// Created by starr on 2024/6/20.
//
#include "LoFiGfx.h"

#include <future>

#include "GfxContext.h"
//#include "PfxContext.h"
#include "PfxContext.h"
#include "SDL3/SDL.h"
#include "entt/entt.hpp"
#include "mimalloc/mimalloc.h"
#include "taskflow/taskflow.hpp"
#include "taskflow/algorithm/for_each.hpp"


void GStart() {
      mi_version();
      mi_option_set(mi_option_verbose, 1);
      mi_option_set(mi_option_show_stats, 1);
      mi_option_set(mi_option_show_errors, 1);
      mi_option_set(mi_option_arena_eager_commit, 2);
      mi_option_set(mi_option_reserve_huge_os_pages, 2);

      const auto vs = R"(
            #extension GL_EXT_nonuniform_qualifier : enable
            #extension GL_EXT_buffer_reference : enable
            #extension GL_EXT_scalar_block_layout : enable
            #extension GL_EXT_buffer_reference2 : enable

            layout(location = 0) in vec3 pos;
            layout(location = 1) in vec3 color;

            layout(location = 0) out vec3 out_pos;

            layout(push_constant) uniform Info{
                  uint tex;
                  float scale;
            } info;

            void VSMain() {
                  gl_Position = vec4(pos, 1.0f);
                  out_pos = pos;
            }
      )";

      const auto ps = R"(
            #extension GL_EXT_nonuniform_qualifier : enable
            #extension GL_EXT_buffer_reference : enable
            #extension GL_EXT_scalar_block_layout : enable
            #extension GL_EXT_buffer_reference2 : enable

            layout(location = 0) in vec3 pos;
            layout(location = 0) out vec4 outColor;

            layout(binding = 0) uniform sampler2D tex[];

            layout(push_constant) uniform Info{
                  uint tex;
                  float scale;
            } info;

            void FSMain() {
                  vec3 get_color = texture(tex[info.tex], pos.xy).rgb;
                  outColor = vec4(get_color, 1.0f) * info.scale;
            }
      )";

      const auto program_config = R"(
            #set rt = r8g8b8a8_unorm
            #set ds = d32_sfloat
            #set color_blend = false
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
            -1.0f, 1.0f, 0.0f, 1.0f,
            1.0f, 1.0f, 1.0f, 1.0f,
            1.0f, -1.0f, 1.0f, 0.0f,
            -1.0f, -1.0f, 0.0f, 0.0f,
      };

      std::array square_id = {0, 1, 2, 0, 2, 3};

      std::vector<float> noise_image(512 * 512 * 4);
      for (int i = 0; i < 512; i++) {
            for (int j = 0; j < 512; j++) {
                  noise_image[i * 512 * 4 + j * 4 + 0] = sin(i / 10.0f) * 0.5f + 0.5f;
                  noise_image[i * 512 * 4 + j * 4 + 1] = cos(j / 10.0f) * 0.5f + 0.5f;
                  noise_image[i * 512 * 4 + j * 4 + 2] = 0.5f;
                  noise_image[i * 512 * 4 + j * 4 + 3] = 1.0f;
            }
      }
      std::unique_ptr<LoFi::GfxContext> ctx;
      try {
            ctx = std::make_unique<LoFi::GfxContext>();
            ctx->Init();
      } catch (const std::runtime_error& e) {
            printf("SDL Init Error: %s\n", e.what());
            return;
      }

      const auto noise = ctx->CreateTexture2D(noise_image, VK_FORMAT_R32G32B32A32_SFLOAT, 512, 512);

      const auto triangle_vert = ctx->CreateBuffer(triangle_vt);
      const auto triangle_index = ctx->CreateBuffer(triangle_id);

      const auto square_vert = ctx->CreateBuffer(square_vt);
      const auto square_index = ctx->CreateBuffer(square_id);

      //CreateWindows
      //const auto win1 = ctx->CreateWindow("Triangle", 960, 540);
      const auto win2 = ctx->CreateWindow("2Draw", 1920 * 0.75, 1080 * 0.75);
      //const auto win3 = ctx->CreateWindow("Merge", 800, 600);

      const auto rt1 = ctx->CreateTexture2D(VK_FORMAT_R8G8B8A8_UNORM, 960, 540);
      const auto rt2 = ctx->CreateTexture2D(VK_FORMAT_R8G8B8A8_UNORM, 1920 * 0.75, 1080 * 0.75);
      const auto med_rt = ctx->CreateTexture2D(VK_FORMAT_R8G8B8A8_UNORM, 960, 540);
      const auto rt3 = ctx->CreateTexture2D(VK_FORMAT_R8G8B8A8_UNORM, 800, 600);

      const auto ds = ctx->CreateTexture2D(VK_FORMAT_D32_SFLOAT, 800, 600);

      //ctx->MapRenderTargetToWindow(rt1, win1);
      ctx->MapRenderTargetToWindow(rt2, win2);
      //ctx->MapRenderTargetToWindow(rt3, win3);

      const auto program = ctx->CreateProgram({vs, ps}, program_config);
      const auto kernel = ctx->CreateKernel(program);

      //const auto font_program = ctx->CreateProgram({font_vs, font_ps});
      //const auto font_kernel = ctx->CreateKernel(font_program);

      float fps = 0;
      auto sum = SDL_GetTicks();
      auto prev = sum;
      std::atomic<bool> should_close = false;

      ctx->SetKernelConstant(kernel, "scale", 0.5f);


      const auto my_cs = R"(
            #extension GL_EXT_nonuniform_qualifier : enable
            #extension GL_EXT_buffer_reference : enable
            #extension GL_EXT_scalar_block_layout : enable
            #extension GL_EXT_buffer_reference2 : enable

            layout(binding = 1, rgba8) uniform readonly image2D inputImage[];
            layout(binding = 1, rgba8) uniform writeonly image2D outputImage[];

            layout(push_constant) uniform Info{
                  uint inTexture;
                  uint outTexture;
                  float scale;
            } info;

            layout(local_size_x = 4, local_size_y = 4, local_size_z = 1) in;

            void CSMain() {
                  vec4 pixel = imageLoad(inputImage[info.inTexture], ivec2(gl_GlobalInvocationID.xy));
                  pixel *= info.scale;
                  imageStore(outputImage[info.outTexture], ivec2(gl_GlobalInvocationID.xy), pixel);
            }
      )";

      const auto cs_program = ctx->CreateProgram({my_cs}, "");
      const auto cs_kernel = ctx->CreateKernel(cs_program);

      ctx->SetKernelConstant(kernel, "scale", 0.8f);
      ctx->SetKernelConstant(cs_kernel, "scale", 0.3f);

      auto pfx = std::make_unique<LoFi::PfxContext>();


      auto func = std::async(std::launch::async, [&] {
            while (!should_close) {
                  float time = (float)((double)SDL_GetTicks() / 1000.0);
                  auto current = SDL_GetTicks();
                  sum += current - prev;
                  prev = current;
                  fps += 1.0f;
                  if (sum > 1'000) {
                        printf("FPS: %f\n", fps);
                        sum = 0;
                        fps = 0.0f;
                  }

                  float multi = (glm::sin(time) * 0.5f + 0.5f) * 0.5f;
                  float multi2 = glm::clamp((glm::sin(time) * 0.5f + 0.5f), 0.0f, 1.0f);

                  pfx->Reset();
                  //pfx->PushScissor({0, 0, 3840 - 2048 * multi, 2160});
                  for (int i = 0; i < 10; i++) {
                        for (int j = 0; j < 10; j++) {
                              pfx->DrawRoundBox(glm::vec2(300, 300) + glm::vec2(200 * j, 150 * i) + glm::vec2(300) * multi,
                              {
                                    .Size = {175, 125},
                                    .RoundnessTopRight = multi2,
                                    .RoundnessBottomRight = 0,
                                    .RoundnessTopLeft = 0,
                                    .RoundnessBottomLeft = 0}, 2.6f * i * j * multi2, {(i + 1) * 20, (j + 1) * 20, 128, 255});
                        }
                  }
                  //pfx->PopScissor();
                  pfx->DrawBox(glm::vec2(2400, 1200), {
                        .Size = {800, 800}
                  },60, {255, 0, 0, 255});

                  pfx->DrawRoundBox(glm::vec2(2400, 400) - multi2 * glm::vec2(300, -300),
                  {
                        .Size = {360 * 2, 640 * 2},
                        .RoundnessTopRight = multi2,
                        .RoundnessBottomRight = multi2,
                        .RoundnessTopLeft = 1.0f - multi2,
                        .RoundnessBottomLeft = 1.0f - multi2
                  }, 60 * multi2, {255, 0, 0, 255});

                  pfx->DrawNGon(glm::vec2(700, 1300) + multi2 * glm::vec2(300, -300),
                  {
                        .Radius = 320,
                        .SegmentCount = 5 + (9 * multi2),
                        .Roundness = (1.0f - multi2)
                  },60* multi2, {255, 0, 0, 255});

                  pfx->DrawCircle(glm::vec2(700, 700) + multi2 * glm::vec2(300, -300),
                {
                      .Radius = 320
                  }, 60* multi2, {255, 0, 0, 255});

                  pfx->DispatchGenerateCommands();


                  ctx->BeginFrame();


                  //Pass 2D

                  ctx->CmdBeginRenderPass({LoFi::RenderPassBeginArgument{.TextureHandle = rt2, .ClearColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)}});
                  pfx->EmitDrawCommand();
                  ctx->CmdEndRenderPass();


                  //Pass 1
                  ctx->CmdBeginRenderPass({{rt1}});

                  ctx->AsSampledTexure(noise);
                  ctx->SetKernelConstant(kernel, "tex", ctx->GetTextureBindlessIndex(noise));

                  ctx->CmdBindKernel(kernel);
                  ctx->CmdBindVertexBuffer(triangle_vert);
                  ctx->CmdBindIndexBuffer(triangle_index);
                  ctx->CmdDrawIndex(triangle_id.size());

                  ctx->CmdEndRenderPass();

                  //CS Stage
                  ctx->BeginComputePass();

                  ctx->AsReadTexure(rt1);
                  ctx->AsWriteTexture(med_rt);

                  ctx->SetKernelConstant(cs_kernel, "inTexture", ctx->GetTextureBindlessIndex(rt1));
                  ctx->SetKernelConstant(cs_kernel, "outTexture", ctx->GetTextureBindlessIndex(med_rt));

                  ctx->CmdBindKernel(cs_kernel);
                  ctx->CmdComputeDispatch(960 / 4, 540 / 4, 1);

                  ctx->EndComputePass();

                  //Pass 2
                  ctx->CmdBeginRenderPass({{rt3}, {ds}});

                  ctx->AsSampledTexure(rt1);
                  ctx->SetKernelConstant(kernel, "tex", ctx->GetTextureBindlessIndex(rt1));

                  ctx->CmdBindKernel(kernel);
                  ctx->CmdBindVertexBuffer(square_vert);
                  ctx->CmdBindIndexBuffer(square_index);
                  ctx->CmdDrawIndex(square_id.size());

                  ctx->CmdBindVertexBuffer(triangle_vert);
                  ctx->CmdBindIndexBuffer(triangle_index);
                  ctx->CmdDrawIndex(triangle_id.size());

                  ctx->CmdEndRenderPass();

                  ctx->EndFrame();
            }
      });

      while (ctx->PollEvent()) {}
      should_close = true;
      func.wait();

      printf("结束");
}

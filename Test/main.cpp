#include <iostream>
#include <vector>
#include <array>
#include <future>
#include "LoFiGfx.h"
#include "SDL3/SDL.h"
#include "SDL3/SDL_vulkan.h"
#include "../Third/glm/glm.hpp"

uint64_t CreateCurfaceCallBack(uint64_t window_handle, uint64_t instance) {
      VkSurfaceKHR surface;
      SDL_Vulkan_CreateSurface((SDL_Window*)window_handle, (VkInstance)instance, nullptr, &surface);
      return (uint64_t)surface;
}

int main() {
      GfxInit();

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
                  vec3 get_color = texture(tex[info.tex], (pos.xy + 1.0f) / 2.0f).rgb;
                  outColor = vec4(get_color, 1.0f) * info.scale + vec4(0.3f, 0.3f, 0.3f, 0.0f);
            }
      )";

      const auto program_config = R"(
            #set rt = r8g8b8a8_unorm
            #set ds = d32_sfloat
            #set color_blend = false
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
      printf("initSuccess");


      const auto noise = GfxCreateTexture2D(noise_image, FORMAT_R32G32B32A32_SFLOAT, 512, 512, 1, "noise");
      const auto triangle_vert = GfxCreateBuffer(triangle_vt, "triangle_vert");
      const auto triangle_index = GfxCreateBuffer(triangle_id, "triangle_id");

      const auto square_vert = GfxCreateBuffer(square_vt, "square_vert");
      const auto square_index = GfxCreateBuffer(square_id, "square_index");

      GfxDestroy(square_vert);
      GfxDestroy(square_index);

      auto win = SDL_CreateWindow("hello", 512, 512, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
      //auto win2 = SDL_CreateWindow("hello", 512, 512, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

      const auto swapchain = GfxCreateSwapChain({.pResourceName = "hello", .AnyHandleForResizeCallback = (uint64_t)win, .PtrOnSwapchainNeedResizeCallback = CreateCurfaceCallBack});
      //const auto swapchain2 = GfxCreateSwapChain({.pResourceName = "hello2", .AnyHandleForResizeCallback = (uint64_t)win2, .PtrOnSwapchainNeedResizeCallback = CreateCurfaceCallBack});


      const auto rt1 = GfxCreateTexture2D(FORMAT_R8G8B8A8_UNORM, 960, 540);
      const auto rt2 = GfxCreateTexture2D(FORMAT_R8G8B8A8_UNORM, 1920 * 0.75, 1080 * 0.75);
      const auto med_rt = GfxCreateTexture2D(FORMAT_R8G8B8A8_UNORM, 512, 512);
      const auto rt3 = GfxCreateTexture2D(FORMAT_R8G8B8A8_UNORM, 800, 600);

      const auto ds = GfxCreateTexture2D(FORMAT_D32_SFLOAT, 800, 600);

      const auto program = GfxCreateProgram({vs, ps}, program_config);
      const auto kernel = GfxCreateKernel(program);

      float fps = 0;
      auto sum = SDL_GetTicks();
      auto prev = sum;


      GfxSetKernelConstant(kernel, "scale", 0.5f);

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

      const auto cs_program = GfxCreateProgram({my_cs}, "");
      const auto cs_kernel = GfxCreateKernel(cs_program);

      GfxSetKernelConstant(kernel, "scale", 0.8f);
      GfxSetKernelConstant(cs_kernel, "scale", 1.5f);

      auto canvas = Gfx2DCreateCanvas();

      Gfx2DLoadFont(canvas, "F:\\font\\llt.ttf");
      auto cyTexture = GfxCreateTexture2DFromFile("D:\\K2.png");

      bool should_close = false;
      SDL_Event event;


      auto node = GfxCreateRDGNode({.pRenderNodeName = "MainNode"});

      auto node2 = GfxCreateRDGNode({.pRenderNodeName = "MainNode2"});
      std::array<const char*, 1> wait_for1 = {"MainNode3"};
      GfxSetRDGNodeWaitFor(node2, {.pNamesRenderNodeWaitFor = wait_for1.data(), .countNamesRenderNodeWaitFor = 1});

      auto node3 = GfxCreateRDGNode({.pRenderNodeName = "MainNode3"});
      std::array<const char*, 1> wait_for2 = {"MainNode"};
      GfxSetRDGNodeWaitFor(node3, {.pNamesRenderNodeWaitFor = wait_for2.data(), .countNamesRenderNodeWaitFor = 1});

      GfxSetRootRDGNode(node);

      while (!should_close) {
            SDL_PollEvent(&event);
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
                  should_close = true;
            }

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

            auto nodec = GfxGetRDGNodeCore(node);
            auto nodec2 = GfxGetRDGNodeCore(node2);
            auto nodec3 = GfxGetRDGNodeCore(node3);

            std::array<GfxInfoRenderPassaAttachment, 1> param{
                  GfxInfoRenderPassaAttachment{swapchain}
            };
            // GfxCmdBeginRenderPass(nodec, {.pAttachments = param.data(), .countAttachments = param.size()});
            // GfxCmdAsSampledTexure(nodec, noise);
            // GfxSetKernelConstant(kernel, "tex", GfxGetTextureBindlessIndex(noise));
            // GfxCmdBindKernel(nodec, kernel);
            // GfxCmdBindVertexBuffer(nodec, square_vert);
            // GfxCmdBindIndexBuffer(nodec, square_index);
            // GfxCmdDrawIndex(nodec, square_id.size());
            // GfxCmdEndRenderPass(nodec);


            // GfxCmdBeginComputePass(nodec3);
            //
            // GfxCmdAsReadTexure(nodec3, swapchain);
            // GfxCmdAsWriteTexture(nodec3, med_rt);
            //
            // GfxSetKernelConstant(cs_kernel, "inTexture", GfxGetTextureBindlessIndex(swapchain));
            // GfxSetKernelConstant(cs_kernel, "outTexture", GfxGetTextureBindlessIndex(med_rt));
            //
            // GfxCmdBindKernel(nodec3, cs_kernel);
            // GfxCmdComputeDispatch(nodec3, 512 / 4, 512 / 4, 1);
            // GfxCmdEndComputePass(nodec3);


            Gfx2DReset(canvas);
            Gfx2DCmdPushCanvasSize(canvas, 3840, 2160);

            for (int i = 0; i < 10; i++) {
                  for (int j = 0; j < 10; j++) {
                        Gfx2DCmdDrawRoundBox(canvas, {300 + 200 * (float)j, 300 + 150 * (float)i}, {
                              .Size = {175, 125},
                              .RoundnessTopRight = multi2,
                              .RoundnessBottomRight = 0,
                              .RoundnessTopLeft = 0,
                              .RoundnessBottomLeft = 0
                        }, 2.6f * i * j * multi2, {(uint8_t)((i + 1) * 20), (uint8_t)((j + 1) * 20), 128, 255});
                  }
            }

            Gfx2DCmdPushStrock(canvas, {
                  .Type = Gfx2DStrockFillType::Radial6,
                  .Radial = {
                        .OffsetX = 0.0f,
                        .OffsetY = 0.0f + multi,
                        .Pos1 = 0.05,
                        .Pos2 = 0.25,
                        .Pos3 = 0.45f,
                        .Pos4 = 0.6f,
                        .Pos5 = 0.75f,
                        .Pos6 = 0.9f,
                        .Color1 = {128, 200, 55, 255},
                        .Color2 = {255, 0, 255, 20},
                        .Color3 = {0, 255, 100, 255},
                        .Color4 = {55, 55, 200, 20},
                        .Color5 = {240, 129, 129, 255},
                        .Color6 = {0, 0, 255, 255},
                  }
            });

            Gfx2DCmdDrawBox(canvas, {2400, 1200}, {
                  .Size = {800, 800}
            }, 60, {255, 0, 0, 255});

            Gfx2DCmdDrawRoundBox(canvas, {2400 - multi2 * 300, 400 + multi2 * 300},
            {
                  .Size = {360 * 2, 640 * 2},
                  .RoundnessTopRight = multi2,
                  .RoundnessBottomRight = multi2,
                  .RoundnessTopLeft = 1.0f - multi2,
                  .RoundnessBottomLeft = 1.0f - multi2
            }, 60 * multi2, {255, 0, 0, 255});

            Gfx2DCmdPushStrock(canvas, {
                  .Type = Gfx2DStrockFillType::Texture,
                  .Texture = {cyTexture}
            });

            Gfx2DCmdDrawNGon(canvas, {700 - multi2 * 1300, 400 + multi2 * 300},
            {
                  .Radius = 320,
                  .SegmentCount = 5 + (9 * multi2),
                  .Roundness = (1.0f - multi2)
            }, 60 * multi2, {255, 0, 0, 255});


            Gfx2DCmdDrawBox(canvas, {1200, 1300}, {
                  .Size = {400, 400},
            }, 0);

            Gfx2DCmdDrawText(canvas, {100, 80}, L"Hello World,你好.", {
                  .Size = 45,
                  .Space = 0,
                  .PxRange = 8 * multi2
            }, {120, 100, 255, 255});
            Gfx2DCmdDrawText(canvas, {100, 170}, L"abcdefghijklmnnopqrstuvwxyz.", {
                  .Size = 45,
                  .Space = 0,
                  .PxRange = 8 * multi2
            }, {120, 100, 255, 255});

            //pfx->PopScissor();
            Gfx2DCmdDrawCircle(canvas, {700 + multi2 * 100, 700 - multi2 * 100}, {
                  .Radius = 320
            }, 0, {255, 0, 0, 255});

            Gfx2DCmdPopStrock(canvas);
            Gfx2DCmdPopStrock(canvas);
            Gfx2DDispatchGenerateCommands(canvas);

            std::array<GfxInfoRenderPassaAttachment, 1> param2{
                  GfxInfoRenderPassaAttachment{
                        .TextureHandle = swapchain,
                        .ClearColorR = 240,
                        .ClearColorG = 240,
                        .ClearColorB= 240,
                        .ClearColorA = 240,
                  }
            };
            GfxCmdBeginRenderPass(nodec, {.pAttachments = param2.data(), .countAttachments = param2.size()});
            Gfx2DEmitDrawCommand(canvas, nodec);
            GfxCmdEndRenderPass(nodec);

            GfxGenFrame();
      }

      GfxClose();
      printf("Over");

      getchar();
      getchar();
      getchar();
      getchar();
      return 0;
}

# LoFi-Kit
低保真的3D开发套件

---
# Components:

## LoFiGfx
一个Vulkan GFX, 用于快速原型开发, 并且带有一个2D绘图库.
### Very Fucking hard to use!

### 2D绘制器参数支持:

G-H = Gradient Linear Horizontal

G-V = Gradient Linear Vertical

G-FP = Gradient Follow Path

G-R = Gradient Center Radial

G-CL = Gradient Linear with Custom Direction

G-CR = Gradient Radial with Custom Center

|                     | Shadow | Soft Anti-Aliasing | Texture | G-H | G-V | G-FP | G-R | G-C | G-CR |
|--------------------:|:------:|:------------------:|:-------:|:---:|:---:|:----:|:---:|:---:|:----:|
|                Path |   -    |         -          |    Y    |  Y  |  Y  |  Y   |  Y  |  Y  |  Y   |
|                Rect |   -    |         -          |    Y    |  Y  |  Y  |  Y   |  Y  |  Y  |  Y   |
|        Rounded Rect |   -    |         -          |    Y    |  Y  |  Y  |  Y   |  Y  |  Y  |  Y   |
|            Triangle |   -    |         -          |    Y    |  Y  |  Y  |  Y   |  Y  |  Y  |  Y   |
|             Polygon |   -    |         -          |    Y    |  Y  |  Y  |  Y   |  Y  |  Y  |  Y   |
|              Circle |   -    |         -          |    Y    |  Y  |  Y  |  Y   |  Y  |  Y  |  Y   |
|               Chord |   -    |         -          |    Y    |  Y  |  Y  |  Y   |  Y  |  Y  |  Y   |
|                 Pie |   -    |         -          |    Y    |  Y  |  Y  |  Y   |  Y  |  Y  |  Y   |
|             Ellipse |   -    |         -          |    Y    |  Y  |  Y  |  Y   |  Y  |  Y  |  Y   |
|                Text |   -    |        SDF         |    -    |  Y  |  Y  |  -   |  Y  |  Y  |  Y   |
|         Path Filled |   Y    |         -          |    Y    |  Y  |  Y  |  -   |  Y  |  Y  |  Y   |
|         Rect Filled |   Y    |         -          |    Y    |  Y  |  Y  |  -   |  Y  |  Y  |  Y   |
| Rounded Rect Filled |   Y    |         -          |    Y    |  Y  |  Y  |  -   |  Y  |  Y  |  Y   |
|     Triangle Filled |   Y    |         -          |    Y    |  Y  |  Y  |  -   |  Y  |  Y  |  Y   |
|      Polygon Filled |   Y    |         -          |    Y    |  Y  |  Y  |  -   |  Y  |  Y  |  Y   |
|       Circle Filled |   Y    |         -          |    Y    |  Y  |  Y  |  -   |  Y  |  Y  |  Y   |
|        Chord Filled |   Y    |         -          |    Y    |  Y  |  Y  |  -   |  Y  |  Y  |  Y   |
|          Pie Filled |   Y    |         -          |    Y    |  Y  |  Y  |  -   |  Y  |  Y  |  Y   |
|      Ellipse Filled |   Y    |         -          |    Y    |  Y  |  Y  |  -   |  Y  |  Y  |  Y   |


```c++
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


      auto win = SDL_CreateWindow("hello", 1920 * 0.75, 1080 * 0.75, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

      const auto swapchain = GfxCreateSwapChain({.pResourceName = "hello", .AnyHandleForResizeCallback = (uint64_t)win, .PtrOnSwapchainNeedResizeCallback = CreateCurfaceCallBack});


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

      Gfx2DLoadFont(canvas, "llt.ttf");
      auto P1 = GfxCreateTexture2DFromFile("1.png");
      auto P2 = GfxCreateTexture2DFromFile("2.png");
      auto P3 = GfxCreateTexture2DFromFile("3.png");
      auto P4 = GfxCreateTexture2DFromFile("4.png");
      auto P5 = GfxCreateTexture2DFromFile("5.png");


      auto node = GfxCreateRDGNode({.pRenderNodeName = "MainNode"});

      // auto node2 = GfxCreateRDGNode({.pRenderNodeName = "MainNode2"});
      // std::array<const char*, 1> wait_for1 = {"MainNode3"};
      // GfxSetRDGNodeWaitFor(node2, {.pNamesRenderNodeWaitFor = wait_for1.data(), .countNamesRenderNodeWaitFor = 1});
      //
      // auto node3 = GfxCreateRDGNode({.pRenderNodeName = "MainNode3"});
      // std::array<const char*, 1> wait_for2 = {"MainNode"};
      // GfxSetRDGNodeWaitFor(node3, {.pNamesRenderNodeWaitFor = wait_for2.data(), .countNamesRenderNodeWaitFor = 1});

      GfxSetRootRDGNode(node);
      auto nodec = GfxGetRDGNodeCore(node);

      std::atomic<bool> should_close = false;
      SDL_Event event;
      auto task = std::async([&]() {
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

                  Gfx2DReset(canvas);
                  Gfx2DCmdPushCanvasSize(canvas, 3840, 2160);

                  float t = glm::clamp(sin(time), 0.0f, 1.0f);
                  Gfx2DCmdPushScissor(canvas, (int)(t * 800), (int)(t * 800), 3840, 2160);

                  for (int i = 0; i < 10; i++) {
                        for (int j = 0; j < 10; j++) {
                              Gfx2DCmdDrawRoundBox(canvas, {100 + 200 * (float)j, 500 + 150 * (float)i}, {
                                    .Size = {175, 125},
                                    .RoundnessTopRight = multi2,
                                    .RoundnessBottomRight = 0,
                                    .RoundnessTopLeft = 0,
                                    .RoundnessBottomLeft = 0
                              }, 2.6f * i * j * multi2, {(uint8_t)((i + 1) * 20), (uint8_t)((j + 1) * 20), 128, 255});
                        }
                  }
                  Gfx2DCmdPopScissor(canvas);

                  Gfx2DCmdPushStrock(canvas, {
                        .Type = Gfx2DStrockFillType::Linear6,
                        .Linear = {
                              .DirectionAngle = 0.0f,
                              .Pos1 = 0.05,
                              .Pos2 = 0.25,
                              .Pos3 = 0.45f,
                              .Pos4 = 0.6f,
                              .Pos5 = 0.75f,
                              .Pos6 = 0.9f,
                              .Color1 = {128, 200, 55, 255},
                              .Color2 = {255, 0, 255, (uint8_t)(255 * multi)},
                              .Color3 = {0, 255, 100, 255},
                              .Color4 = {55, 55, 200, (uint8_t)(255 * multi)},
                              .Color5 = {240, 129, 129, 255},
                              .Color6 = {0, 0, 255, 255},
                        }
                  });

                  Gfx2DCmdDrawRoundBox(canvas, {2400, 1600}, {
                        .Size = {500, 300},
                        .RoundnessTopRight = multi2,
                        .RoundnessBottomRight = multi2,
                        .RoundnessTopLeft = 1.0f - multi2,
                        .RoundnessBottomLeft = 1.0f - multi2
                  }, 0, {255, (uint8_t)(255 * multi), 0, 255});

                  Gfx2DCmdPopStrock(canvas);
                  Gfx2DCmdPushStrock(canvas, {
                        .Type = Gfx2DStrockFillType::Radial6,
                        .Radial = {
                              .OffsetX = 0.0f,
                              .OffsetY = 0.0f + multi,
                              .Pos1 = 0.05,
                              .Pos2 = 0.25f + multi * 0.5f,
                              .Pos3 = 0.45f + multi * 1.0f,
                              .Pos4 = 0.6f + multi * 1.5f,
                              .Pos5 = 0.75f + multi * 2.0f,
                              .Pos6 = 0.9f + multi * 2.0f,
                              .Color1 = {128, 200, 55, 255},
                              .Color2 = {255, 0, 255, (uint8_t)(255 * multi)},
                              .Color3 = {0, 255, 100, 255},
                              .Color4 = {55, 55, 200, (uint8_t)(255 * multi)},
                              .Color5 = {240, 129, 129, 255},
                              .Color6 = {0, 0, 255, 255},
                        }
                  });

                  Gfx2DCmdDrawRoundBox(canvas, {2900, 1200 + sin(time - 0.6f) * 200},
                  {
                        .Size = {600, 300},
                        .RoundnessTopRight = multi2,
                        .RoundnessBottomRight = multi2,
                        .RoundnessTopLeft = 1.0f - multi2,
                        .RoundnessBottomLeft = 1.0f - multi2
                  }, 60 * multi2, {255, 0, 0, 255});

                  Gfx2DCmdPushStrock(canvas, {
                        .Type = Gfx2DStrockFillType::Texture,
                        .Texture = {P1}
                  });

                  Gfx2DCmdDrawNGon(canvas, {600, 400 + sin(time - 0.3f) * 200},
                  {
                        .Radius = 240,
                        .SegmentCount = 5 + (9 * multi2),
                        .Roundness = (1.0f - multi2)
                  }, 0, {255, 0, 0, 255});


                  Gfx2DCmdPopStrock(canvas);
                  Gfx2DCmdPushStrock(canvas, {
                        .Type = Gfx2DStrockFillType::Texture,
                        .Texture = {P2}
                  });

                  Gfx2DCmdDrawNGon(canvas, {1200, 400 + sin(time) * 200},
                  {
                        .Radius = 240,
                        .SegmentCount = 4 + (4 * glm::clamp(sin(time), 0.0f, 1.0f)),
                        .Roundness = 0.0f
                  }, 30 * multi2, {255, 0, 0, 255});

                  Gfx2DCmdPopStrock(canvas);
                  Gfx2DCmdPushStrock(canvas, {
                        .Type = Gfx2DStrockFillType::Texture,
                        .Texture = {P3}
                  });

                  Gfx2DCmdDrawNGon(canvas, {1800, 400 + sin(time + 0.3f) * 200},
                  {
                        .Radius = 240,
                        .SegmentCount = 4 + (4 * glm::clamp(sin(time), 0.0f, 1.0f)),
                        .Roundness = 1.0f
                  }, 60 * multi2, {255, 0, 0, 255});

                  Gfx2DCmdPopStrock(canvas);
                  Gfx2DCmdPushStrock(canvas, {
                        .Type = Gfx2DStrockFillType::Texture,
                        .Texture = {P4}
                  });

                  Gfx2DCmdDrawNGon(canvas, {2400, 400 + sin(time + 0.6f) * 200},
                  {
                        .Radius = 240,
                        .SegmentCount = 4 + (4 * glm::clamp(sin(time), 0.0f, 1.0f)),
                        .Roundness = 0.0f
                  }, 0 * multi2, {255, 0, 0, 255});

                  Gfx2DCmdPopStrock(canvas);
                  Gfx2DCmdPushStrock(canvas, {
                        .Type = Gfx2DStrockFillType::Texture,
                        .Texture = {P5}
                  });

                  Gfx2DCmdDrawNGon(canvas, {3000, 400 + sin(time + 0.9f) * 200},
                  {
                        .Radius = 240,
                        .SegmentCount = 4 + (4 * glm::clamp(sin(time), 0.0f, 1.0f)),
                        .Roundness = 0.0f
                  }, 0 * multi2, {255, 0, 0, 255});


                  Gfx2DCmdDrawBox(canvas, {1200, 1300}, {
                        .Size = {400, 400},
                  }, 180 * multi2);

                  Gfx2DCmdDrawText(canvas, {50, 50}, L"Hello World,你好?", {
                        .Size = 45,
                        .Space = 0,
                        .PxRange = 0.5f + 4 * multi2
                  }, {120, 100, 255, 255});
                  Gfx2DCmdDrawText(canvas, {50, 140}, L"无法浏览R-18作品.", {
                        .Size = 45,
                        .Space = 0,
                        .PxRange = 0.5f + 4 * multi2
                  }, {120, 100, 255, 255});

                  //pfx->PopScissor();
                  Gfx2DCmdDrawCircle(canvas, {3400 + multi2 * 200, 700 - multi2 * 100}, {
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
                              .ClearColorB = 240,
                              .ClearColorA = 240,
                        }
                  };
                  GfxCmdBeginRenderPass(nodec, {.pAttachments = param2.data(), .countAttachments = param2.size()});
                  Gfx2DEmitDrawCommand(canvas, nodec);
                  GfxCmdEndRenderPass(nodec);

                  GfxGenFrame();
            }
      });

      while (!should_close) {
            SDL_PollEvent(&event);
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
                  should_close = true;
            }
      }

      task.wait();

      GfxClose();
 
```

----

## LoFiVDB
// TODO : 一个对OpenVDB的包装

## LoFi MeshCore
// TODO : 一个基于Lagrange的建模引擎

## LoFi CXG Engine
// TODO: 一个引擎



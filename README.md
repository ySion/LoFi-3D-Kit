# LoFi-Kit
低保真的3D开发套件, 目前正在开发中

---
# Components:

## LoFiGfx
一个Vulkan GFX, 用于快速原型开发, 并且带有一个2D绘图库.
### Very easy to use!

```c++
 const char* vs = R"(
        #set topology   = triangle_list
        #set polygon_mode = fill
        #set cull_mode   = none
        #set depth_test  = less_or_equal
        #set depth_write  = true

        #set vs_location = 0 0 r32g32b32_sfloat 0
        layout(location = 0) in vec3 pos;

        #set vs_location = 0 1 r32g32b32_sfloat 12
        layout(location = 1) in vec3 color;

        #set vs_binding = 0 24 vertex

        layout(location = 0) out vec3 out_color;

        Struct2(
        Info, {
                 float time;
                 float time2;
                 float time3;
                 float time4;
        },
        Info2, {
                 vec3 pos_adder;
                 vec3 norm_adder;
        }
        );

        void VSMain() {
              float extend = sin(GetVar(Info).time4 * 10.0f) / 5.0;
              gl_Position = vec4(pos, 1.0f) + vec4(extend, extend, 0, 0);
              float t = sin(GetVar(Info).time * 10.0f) / 2.0f;
              out_color = color + vec3(t,t,t);
        }
  )";

  const char* ps = R"(
        #set rt = r8g8b8a8_unorm
        #set ds = d32_sfloat
        #set color_blend = false

        layout(location = 0) in vec3 color;
        layout(location = 0) out vec4 outColor;

        Struct2(
        Info, {
                 float time;
                 float time2;
                 float time3;
                 float time4;
        },
        Info2, {
                 vec3 pos_adder;
                 vec3 norm_adder;
        }
        );

        void FSMain() {
              outColor = vec4(color - GetVar(Info).time3, 1.0f);
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

  const auto win1 = ctx->CreateWindow("Triangle", 1920, 1080);
  const auto win2 = ctx->CreateWindow("Rectangle", 400, 400);
  const auto win3 = ctx->CreateWindow("Depth", 800, 600);

  const auto rt1 = ctx->CreateTexture2D(VK_FORMAT_R8G8B8A8_UNORM, 1920, 1080);
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

  const auto kernel_instance = ctx->CreateGraphicsKernelInstance(kernel);

  std::atomic<bool> should_close = false;
  auto func = std::async(std::launch::async, [&] {
        while (!should_close) {
              float time = (float)((double)SDL_GetTicks() / 1000.0);
              ctx->SetGraphicKernelInstanceParamterStructMember(kernel_instance, "Info.time", time * 2);
              ctx->SetGraphicKernelInstanceParamterStructMember(kernel_instance, "Info.time4", time);
              ctx->SetGraphicKernelInstanceParamterStructMember(kernel_instance, "Info.time3", 0.1f);

              ctx->BeginFrame();

              //Pass 1
              ctx->CmdBeginRenderPass({{rt1}});
              ctx->CmdBindGraphicKernelToRenderPass(kernel_instance);
              ctx->CmdBindVertexBuffer(triangle_vert);
              ctx->CmdDrawIndex(triangle_index);
              ctx->CmdEndRenderPass();

              //Pass 2
              ctx->CmdBeginRenderPass({{rt2}});
              ctx->CmdBindGraphicKernelToRenderPass(kernel_instance);
              ctx->CmdBindVertexBuffer(square_vert);
              ctx->CmdDrawIndex(square_index);
              ctx->CmdEndRenderPass();

              //Pass 3
              ctx->CmdBeginRenderPass({{rt3}, {ds}});
              ctx->CmdBindGraphicKernelToRenderPass(kernel_instance);
              ctx->CmdBindVertexBuffer(square_vert);
              ctx->CmdDrawIndex(square_index);
              ctx->CmdBindVertexBuffer(triangle_vert);
              ctx->CmdDrawIndex(triangle_index);
              ctx->CmdEndRenderPass();

              ctx->EndFrame();
        }
  });

  while (ctx->PollEvent()) {}
  should_close = true;
  func.wait();
```

![image](https://github.com/ySion/LoFi-3D-Kit/assets/39912009/f7defe0b-1da2-41aa-a614-5191097aed3b)

----

## LoFiVDB
// TODO : 一个对OpenVDB的包装

## LoFi MeshCore
// TODO : 一个基于Lagrange的建模引擎

## LoFi CXG Engine
// TODO: 一个引擎



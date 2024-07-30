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

            SAMPLED image1;

            RBUFFER Info {
                  float scale;
            }

            void FSMain() {
                  uint image_hanlde = GetTextureID(image1);
                  vec3 f = texture(GetSampled2D(image_hanlde), vec2(pos.x, pos.y)).rgb;
                  outColor = vec4(f, 1.0f) * (sin(GetBuffer(Info).scale) * 0.5 + 0.5);
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

      auto ctx = std::make_unique<LoFi::Context>();
      ctx->Init();

      const auto noise = ctx->CreateTexture2D(noise_image, VK_FORMAT_R32G32B32A32_SFLOAT, 512, 512);

      const auto triangle_vert = ctx->CreateBuffer(triangle_vt);
      const auto triangle_index = ctx->CreateBuffer(triangle_id);

      const auto square_vert = ctx->CreateBuffer(square_vt);
      const auto square_index = ctx->CreateBuffer(square_id);

      //CreateWindows
      const auto win1 = ctx->CreateWindow("Triangle", 960, 540);
      const auto win2 = ctx->CreateWindow("Rectangle", 400, 400);
      const auto win3 = ctx->CreateWindow("Merge", 800, 600);

      const auto rt1 = ctx->CreateTexture2D(VK_FORMAT_R8G8B8A8_UNORM, 960, 540);
      const auto rt2 = ctx->CreateTexture2D(VK_FORMAT_R8G8B8A8_UNORM, 400, 400);
      const auto rt3 = ctx->CreateTexture2D(VK_FORMAT_R8G8B8A8_UNORM, 800, 600);

      const auto ds = ctx->CreateTexture2D(VK_FORMAT_D32_SFLOAT, 800, 600);

      ctx->MapRenderTargetToWindow(rt1, win1);
      ctx->MapRenderTargetToWindow(rt2, win2);
      ctx->MapRenderTargetToWindow(rt3, win3);

      const auto program = ctx->CreateProgram({vs, ps});
      const auto kernel = ctx->CreateKernel(program);
      const auto kernel_instance = ctx->CreateKernelInstance(kernel);

      const auto kernel_instance2 = ctx->CreateKernelInstance(kernel);

      const auto info_param = ctx->CreateFrameResource(sizeof(float));

      ctx->BindKernelResource(kernel_instance, "Info", info_param);
      ctx->BindKernelResource(kernel_instance2, "Info", info_param);

      ctx->BindKernelResource(kernel_instance, "image1", noise);
      ctx->BindKernelResource(kernel_instance2, "image1", rt1);

      std::atomic<bool> should_close = false;
      auto func = std::async(std::launch::async, [&] {
            while (!should_close) {
                  float time = (float)((double)SDL_GetTicks() / 1000.0);
                  ctx->SetFrameResourceData(info_param, &time, sizeof(float));
                  ctx->BeginFrame();

                  //Pass 1
                  ctx->CmdBeginPass({{rt1}});
                  ctx->CmdBindKernel(kernel_instance);
                  ctx->CmdBindVertexBuffer(triangle_vert);
                  ctx->CmdDrawIndex(triangle_index);
                  ctx->CmdEndPass();

                  //Pass 2
                  ctx->CmdBeginPass({{rt2}});
                  ctx->CmdBindKernel(kernel_instance2);
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
 
```

![image](https://github.com/ySion/LoFi-3D-Kit/assets/39912009/f7defe0b-1da2-41aa-a614-5191097aed3b)
![image](https://github.com/ySion/LoFi-3D-Kit/assets/39912009/57b8480f-66b2-472e-9680-7464c720e5d8)


----

## LoFiVDB
// TODO : 一个对OpenVDB的包装

## LoFi MeshCore
// TODO : 一个基于Lagrange的建模引擎

## LoFi CXG Engine
// TODO: 一个引擎



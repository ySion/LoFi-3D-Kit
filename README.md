# LoFi-Kit
低保真的3D开发套件, 目前正在开发中

---
# Components:

## LoFiGfx
一个Vulkan GFX, 用于快速原型开发, 并且带有一个2D绘图库.
### Very Fucking hard to use!

```c++
       const char* vs = R"(

            layout(location = 0) in vec3 pos;
            layout(location = 1) in vec3 color;

            layout(location = 0) out vec3 out_color;
            layout(location = 1) out vec3 out_pos;

            STRUCTEXT Info {
                  float time;
                  float time2;
                  float time3;
                  float time4;
            }

            void VSMain() {
                  float extendx = cos(GetVar(Info).time4 * 10.0f) / 5.0;
                  float extendy = sin(GetVar(Info).time4 * 10.0f) / 5.0;
                  gl_Position = vec4(pos, 1.0f) + vec4(extendx, extendy, 0, 0);
                  out_pos = pos+ vec3(extendx, extendy, 0);
                  float t = sin(GetVar(Info).time * 10.0f) / 2.0f;
                  out_color = color + vec3(t,t,t);
            }
      )";

      const char* ps = R"(

            #set rt = r8g8b8a8_unorm
            #set ds = d32_sfloat

            #set color_blend = add

            layout(location = 0) in vec3 color;
            layout(location = 1) in vec3 pos;

            layout(location = 0) out vec4 outColor;

            STRUCTEXT Info {
                  float time;
                  float time2;
                  float time3;
                  float time4;
            }

            TEXTURE some_texture;
            TEXTURE some_texture2;

            void FSMain() {
                  vec3 f = texture(GetTex2D(some_texture), vec2(pos.x, pos.y)).rgb;
                  vec3 f2 = texture(GetTex2D(some_texture2), vec2(pos.x, pos.y)).rgb;
                  vec3 f3 = f * f2;
                  outColor =  vec4(f, 0.5f);
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

      std::vector<float> noise_image(256 * 256 * 4);
      std::vector<float> noise_image2(64 * 64 * 4);

      for(int i = 0; i < 256; i++) {
            for(int j = 0; j < 256; j++) {
                  noise_image[i*256*4 + j*4 + 0] = sin(i / 10.0f) * 0.5f + 0.5f;
                  noise_image[i*256*4 + j*4 + 1] = sin(j / 10.0f) * 0.5f + 0.5f;
                  noise_image[i*256*4 + j*4 + 2] = 1.0f;
                  noise_image[i*256*4 + j*4 + 3] = 1.0f;
            }
      }

      for(int i = 0; i < 64; i++) {
            for(int j = 0; j < 64; j++) {
                  noise_image2[i*64*4 + j*4 + 0] = cos(i / 5.0f) * 0.5f + 0.5f;
                  noise_image2[i*64*4 + j*4 + 1] = cos(j / 5.0f) * 0.5f + 0.5f;
                  noise_image2[i*64*4 + j*4 + 2] = 0.0f;
                  noise_image2[i*64*4 + j*4 + 3] = 1.0f;
            }
      }


      auto ctx = std::make_unique<LoFi::Context>();
      ctx->Init();

      const auto noise = ctx->CreateTexture2D(noise_image, VK_FORMAT_R32G32B32A32_SFLOAT, 256, 256);
      const auto noise2 = ctx->CreateTexture2D(noise_image2, VK_FORMAT_R32G32B32A32_SFLOAT, 64, 64);

      const auto triangle_vert = ctx->CreateBuffer(triangle_vt);
      const auto triangle_index = ctx->CreateBuffer(triangle_id);

      const auto square_vert = ctx->CreateBuffer(square_vt);
      const auto square_index = ctx->CreateBuffer(square_id);

      //CreateWindows
      const auto win1 = ctx->CreateWindow("Triangle", 1920, 1080);
      const auto win2 = ctx->CreateWindow("Rectangle", 400, 400);
      const auto win3 = ctx->CreateWindow("Merge", 800, 600);

      const auto rt1 = ctx->CreateTexture2D(VK_FORMAT_R8G8B8A8_UNORM, 1920, 1080);
      const auto rt2 = ctx->CreateTexture2D(VK_FORMAT_R8G8B8A8_UNORM, 400, 400);
      const auto rt3 = ctx->CreateTexture2D(VK_FORMAT_R8G8B8A8_UNORM, 800, 600);

      const auto ds = ctx->CreateTexture2D(VK_FORMAT_D32_SFLOAT, 800, 600);

      ctx->MapRenderTargetToWindow(rt1, win1);
      ctx->MapRenderTargetToWindow(rt2, win2);
      ctx->MapRenderTargetToWindow(rt3, win3);

      //Compile Shader and Create "Material"
      const auto program = ctx->CreateProgram({vs, ps});
      const auto kernel = ctx->CreateGraphicKernel(program);

      //Create "Material Intance"
      const auto kernel_instance = ctx->CreateGraphicsKernelInstance(kernel);

      //Set "Material Intance" Paramter
      ctx->SetKernelTexture(kernel_instance, "some_texture", noise);
      ctx->SetKernelTexture(kernel_instance, "some_texture2", noise2);

      std::atomic<bool> should_close = false;
      auto func = std::async(std::launch::async, [&] {
            while (!should_close) {
                  float time = (float)((double)SDL_GetTicks() / 1000.0);
                  ctx->SetKernelParamter(kernel_instance, "Info.time", time * 2);
                  ctx->SetKernelParamter(kernel_instance, "Info.time4", time);
                  ctx->SetKernelParamter(kernel_instance, "Info.time3", 0.0f);

                  ctx->BeginFrame();

                  //Pass 1
                  ctx->CmdBeginRenderPass({{rt1}});
                  ctx->CmdBindKernel(kernel_instance);
                  ctx->CmdBindVertexBuffer(triangle_vert);
                  ctx->CmdDrawIndex(triangle_index);
                  ctx->CmdEndRenderPass();

                  //Pass 2
                  ctx->CmdBeginRenderPass({{rt2}});
                  ctx->CmdBindKernel(kernel_instance);
                  ctx->CmdBindVertexBuffer(square_vert);
                  ctx->CmdDrawIndex(square_index);
                  ctx->CmdEndRenderPass();

                  //Pass 3
                  ctx->CmdBeginRenderPass({{rt3}, {ds}});
                  ctx->CmdBindKernel(kernel_instance);
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
![image](https://github.com/ySion/LoFi-3D-Kit/assets/39912009/57b8480f-66b2-472e-9680-7464c720e5d8)


----

## LoFiVDB
// TODO : 一个对OpenVDB的包装

## LoFi MeshCore
// TODO : 一个基于Lagrange的建模引擎

## LoFi CXG Engine
// TODO: 一个引擎



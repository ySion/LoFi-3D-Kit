# LoFi-Kit
低保真的3D开发套件, 目前正在开发中

---
# Components:

## LoFiGfx
一个Vulkan GFX, 用于快速原型开发, 并且带有一个2D绘图库.
### Very easy to use!

```c++
    //triangle
    std::array triangle_vt = {
          0.0f, 0.5f,  1.0f, 0.0f, 0.0f,
          0.5f, -0.5f,   0.0f, 1.0f, 0.0f,
          -0.5f, -0.5f,   0.0f, 0.0f, 1.0f,
    };
    
    std::array<uint32_t, 3> triangle_id = {0, 1, 2,};
    
    //square
    std::array square_vt = {
          -0.5f, 0.5f,  1.0f, 0.0f, 0.0f,
          0.5f, 0.5f,   0.0f, 1.0f, 0.0f,
          0.5f, -0.5f,   0.0f, 0.0f, 1.0f,
          -0.5f, -0.5f,   1.0f, 1.0f, 1.0f,
    };
    
    std::array<uint32_t, 6> square_id = {0, 1, 2, 0, 2, 3};
    
    std::array rect_vt = {
          -0.8f, 0.5f,  1.0f, 0.0f, 0.0f,
          0.8f, 0.5f,   0.0f, 1.0f, 0.0f,
          0.8f, -0.5f,   0.0f, 0.0f, 1.0f,
          -0.8f, -0.5f,   1.0f, 1.0f, 1.0f,
    };
    
    std::array<uint32_t, 6> rect_id = {0, 1, 2, 0, 2, 3};

    const char* vs = R"(
          #set topology   = triangle_list
          #set polygon_mode = fill
          #set cull_mode   = none

          #set vs_location = 0 0 r32g32_sfloat 0
          layout(location = 0) in vec2 pos;

          #set vs_location = 0 1 r32g32b32_sfloat 12
          layout(location = 1) in vec3 color;

          #set vs_binding = 0 20 vertex

          layout(location = 0) out vec3 out_color;

          void VSMain() {
                gl_Position = vec4(pos, 0.0f, 1.0f);
                out_color = color;
          }
    )";
    
    const char* ps = R"(
          #set rt = r8g8b8a8_unorm
          #set color_blend = false
    
          layout(location = 0) in vec3 color;
          layout(location = 0) out vec4 outColor;
    
          void FSMain() {
                outColor = vec4(color, 1.0f);
          }
    )";
    
    auto ctx = std::make_unique<LoFi::Context>();
    ctx->Init();

    auto win1 = ctx->CreateWindow("window1", 800, 600);
    auto win2 = ctx->CreateWindow("window2", 400, 400);
    auto win3 = ctx->CreateWindow("window3", 800, 600);
    
    auto rt1 = ctx->CreateTexture2D(VK_FORMAT_R8G8B8A8_UNORM, 800, 600);
    auto rt2 = ctx->CreateTexture2D(VK_FORMAT_R8G8B8A8_UNORM, 400, 400);
    auto rt3 = ctx->CreateTexture2D(VK_FORMAT_R8G8B8A8_UNORM, 800, 600);
    
    ctx->MapRenderTargetToWindow(rt1, win1);
    ctx->MapRenderTargetToWindow(rt2, win2);
    ctx->MapRenderTargetToWindow(rt3, win3);
    
    auto triangle_vert = ctx->CreateBuffer(triangle_vt.data(), triangle_vt.size() * sizeof(float));
    auto triangle_index = ctx->CreateBuffer(triangle_id.data(), triangle_id.size() * sizeof(uint32_t));
    
    auto square_vert = ctx->CreateBuffer(square_vt.data(), square_vt.size() * sizeof(float));
    auto square_index = ctx->CreateBuffer(square_id.data(), square_id.size() * sizeof(uint32_t));
    
    auto rect_vert = ctx->CreateBuffer(rect_vt.data(), rect_vt.size() * sizeof(float));
    auto rect_index = ctx->CreateBuffer(rect_id.data(), rect_id.size() * sizeof(uint32_t));
    
    auto program = ctx->CreateProgram({vs, ps});
    auto kernel = ctx->CreateGraphicKernel(program);
    
    std::atomic<bool> should_close = false;
    auto func = std::async(std::launch::async, [&] {
          while (!should_close) {
                ctx->BeginFrame();
    
                ctx->CmdBindRenderTargetBeforeRenderPass(rt1);
                ctx->CmdBeginRenderPass();
                ctx->CmdBindGraphicKernelToRenderPass(kernel);
                ctx->CmdBindVertexBuffer(triangle_vert);
                ctx->CmdDrawIndex(triangle_index);
                ctx->CmdEndRenderPass();
    
                ctx->CmdBindRenderTargetBeforeRenderPass(rt2);
                ctx->CmdBeginRenderPass();
                ctx->CmdBindGraphicKernelToRenderPass(kernel);
                ctx->CmdBindVertexBuffer(square_vert);
                ctx->CmdDrawIndex(square_index);
                ctx->CmdEndRenderPass();
    
                ctx->CmdBindRenderTargetBeforeRenderPass(rt3);
                ctx->CmdBeginRenderPass();
                ctx->CmdBindGraphicKernelToRenderPass(kernel);
                ctx->CmdBindVertexBuffer(rect_vert);
                ctx->CmdDrawIndex(rect_index);
                ctx->CmdEndRenderPass();
    
                ctx->EndFrame();
          }
    });
    
    while (ctx->PollEvent()) {}

    should_close = true;
    
    func.wait();
```

![0c3821d0251d2c4aee7c429b19555ee5](https://github.com/ySion/LoFi-3D-Kit/assets/39912009/2d556724-f78a-4b8e-8c85-512899ee45b2)

----

## LoFiVDB
// TODO : 一个对OpenVDB的包装

## LoFi MeshCore
// TODO : 一个基于Lagrange的建模引擎

## LoFi CXG Engine
// TODO: 一个引擎



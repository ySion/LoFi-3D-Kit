//
// Created by starr on 2024/6/20.
//
#include "LoFiGfx.h"
#include <iostream>
#include "Context.h"

void GStart() {

      LoFi::Context ctx{};
      ctx.EnableDebug();
      ctx.Init();
      ctx.CreateWindow("hello world", 800, 600);

      auto renderTexture = ctx.CreateRenderTexture(800, 600);
      auto renderTexture2 = ctx.CreateRenderTexture(800, 600);


      while(ctx.PollEvent()) {

      }


      ctx.Shutdown();
      printf("Hello world");
}

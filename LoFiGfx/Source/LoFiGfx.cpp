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
      printf("Hello world");
}

//
// Created by starr on 2024/6/20.
//

#ifndef WINDOW_H
#define WINDOW_H

#include <memory>
#include <vector>

#include "Marcos.h"
#include "Texture.h"

struct SDL_Window;

namespace LoFi {

class Window {
public:
      NO_COPY_MOVE_CONS(Window)

      explicit Window(const char* title, int width, int height);

      ~Window();

      void Resize(int w, int h) const;

      void SetFullScreen(bool full_screen);

      void Update();

      int32_t GetWindowID() const;

private:
      void CreateOrRecreateSwapChain();

      int _w {};
      int _h {};

      SDL_Window* _window {};

      VkSurfaceKHR _surface {};
      VkSwapchainKHR _swapchain {};

      std::vector<std::unique_ptr<Texture>> _images{};
};

} // LoFi

#endif //WINDOW_H

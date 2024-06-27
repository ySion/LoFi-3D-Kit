#include "Window.h"
#include "Swapchain.h"
#include "SDL3/SDL.h"
#include "../Context.h"

using namespace LoFi::Component;
using namespace LoFi::Internal;

Window::Window(entt::entity id, const char* title, int w, int h) : _id(id) {
      _window = SDL_CreateWindow(title, w, h, SDL_WINDOW_VULKAN);
      SDL_SetWindowMinimumSize(_window, 64, 64);
      SDL_SetWindowResizable(_window, true);

      entt::registry& reg = *volkGetLoadedEcsWorld();
      reg.emplace<Swapchain>(_id, _id);
}

Window::~Window() {
      entt::registry& reg = *volkGetLoadedEcsWorld();

      if (reg.try_get<Swapchain>(_id)) {
            reg.remove<Swapchain>(_id);
      }

      SDL_DestroyWindow(_window);
}

void Window::Resize(int x, int y) const {
      SDL_SetWindowSize(_window, x, y);
}

VkExtent2D Window::GetSize() const {
      int x, y;
      SDL_GetWindowSize(_window, &x, &y);
      return {(uint32_t)x, (uint32_t)y};
}

SDL_Window* Window::GetWindowPtr() const {
      return _window;
}

uint32_t Window::GetWindowID() const {
      return SDL_GetWindowID(_window);
}

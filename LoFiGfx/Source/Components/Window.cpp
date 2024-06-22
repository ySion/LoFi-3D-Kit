#include "Window.h"
#include "Swapchain.h"
#include "SDL3/SDL.h"
#include "../Context.h"

using namespace LoFi::Component;

Window::Window(entt::entity id, const char* title, int w, int h) : _id(id), _w(w), _h(h) {
      _window = SDL_CreateWindow(title, _w, _h, SDL_WINDOW_VULKAN);
      SDL_SetWindowMinimumSize(_window, 64, 64);
      SDL_SetWindowResizable(_window, true);

      entt::registry& reg = *volkGetLoadedEcsWorld();
      reg.emplace<Swapchain>(_id, _id);
}

Window::~Window()
{
      entt::registry& reg = *volkGetLoadedEcsWorld();

      if(reg.try_get<Swapchain>(_id)) {
            reg.remove<Swapchain>(_id);
      }

      SDL_DestroyWindow(_window);
}

void Window::Resize(int x, int y) const {
      SDL_SetWindowSize(_window, x, y);
}

SDL_Window* Window::GetWindowPtr() const {
      return _window;
}

uint32_t Window::GetWindowID() const {
      return SDL_GetWindowID(_window);
}

void Window::Update() {

      int x, y;
      SDL_GetWindowSize(_window, &x, &y);
      if (x != _w || y != _h) {
            _w = x;
            _h = y;
            auto world = volkGetLoadedEcsWorld();
      }
}


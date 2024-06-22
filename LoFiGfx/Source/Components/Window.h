#pragma once

#include "../Helper.h"
#include "SDL3/SDL_video.h"

struct SDL_Window;

namespace LoFi::Component {

      class Window {
      public:
            NO_COPY_MOVE_CONS(Window);

            Window(entt::entity id, const char* title, int w, int h);

            ~Window();

            entt::entity GetID() const { return _id; }

            void Resize(int x, int y) const;

            VkExtent2D GetSize() const { return { _w, _h }; }

            SDL_Window* GetWindowPtr() const;

            uint32_t GetWindowID() const;

      public:

            void Update();

      private:
            entt::entity _id{};

            uint32_t _w {};

            uint32_t _h {};

            SDL_Window* _window {};

      private:
            friend class Context;
      };
}
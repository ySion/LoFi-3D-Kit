//
// Created by starr on 2024/6/20.
//
#define VK_NO_PROTOTYPES

#include "Marcos.h"

#include "VmaLoader.h"


namespace LoFi {

      class Context {
      public:
            Context() = default;

            ~Context();

            NO_COPY_MOVE_CONS(Context)

            void EnableDebug();

            void Init();

            void Shutdown();

      private:
            bool _bDebugMode = false;

            VkInstance _instance;
            VkPhysicalDevice _physicalDevice;
            VkDevice _device;
            VmaAllocator _allocator;

      };
}

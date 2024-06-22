//
// Created by starr on 2024/6/20.
//

#include "Helper.h"

static VmaAllocator loadedAllocator = VK_NULL_HANDLE;
static entt::registry* loadedEcsWorld = nullptr;

VmaAllocator volkGetLoadedVmaAllocator() {
      return loadedAllocator;
}

void volkLoadVmaAllocator(VmaAllocator allocator) {
      loadedAllocator = allocator;
}

entt::registry* volkGetLoadedEcsWorld() {
      return loadedEcsWorld;
}

void volkLoadEcsWorld(entt::registry* world) {
      loadedEcsWorld = world;
}


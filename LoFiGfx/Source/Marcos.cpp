//
// Created by starr on 2024/6/20.
//

#include "Marcos.h"

static VmaAllocator loadedAllocator = VK_NULL_HANDLE;

VmaAllocator volkGetLoadedVmaAllocator() {
      return loadedAllocator;
}

void volkLoadVmaAllocator(VmaAllocator allocator) {
      loadedAllocator = allocator;
}


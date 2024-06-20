//
// Created by starr on 2024/6/20.
//

#ifndef MARCOS_H
#define MARCOS_H

#define NO_COPY_MOVE_CONS(Class) \
Class(const Class&) = delete; \
Class(Class&&) = delete; \
Class& operator=(const Class&) = delete; \
Class& operator=(Class&&) = delete;

#include "../Third/volk/volk.h"
#include "VmaLoader.h"

VmaAllocator volkGetLoadedVmaAllocator();
void volkLoadVmaAllocator(VmaAllocator allocator);

#endif //MARCOS_H

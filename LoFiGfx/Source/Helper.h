//
// Created by starr on 2024/6/20.
//

#ifndef MARCOS_H
#define MARCOS_H

#define NO_COPY_MOVE_CONS(Class) \
Class(const Class&) = delete; \
Class(Class&&) = delete; \
Class& operator=(const Class&) = delete; \
Class& operator=(Class&&) = delete

#define NO_COPY(Class) \
Class(const Class&) = delete; \
Class& operator=(const Class&) = delete

#include <format>
#include <string>
#include <stdexcept>
#include <functional>
#include <vector>

#include "../Third/volk/volk.h"
#include "VmaLoader.h"
#include "entt/entt.hpp"

VmaAllocator volkGetLoadedVmaAllocator();
void volkLoadVmaAllocator(VmaAllocator allocator);

entt::registry* volkGetLoadedEcsWorld();
void volkLoadEcsWorld(entt::registry* world);

#endif //MARCOS_H

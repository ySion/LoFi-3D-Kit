cmake_minimum_required(VERSION 3.28)

project(Test)


add_executable(Test main.cpp)
target_link_libraries(Test PRIVATE LoFiGfx SDL3::SDL3 glm::glm-header-only)

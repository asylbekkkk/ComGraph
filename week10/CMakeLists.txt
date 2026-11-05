cmake_minimum_required(VERSION 3.10)
project(week10)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)

# GLAD репозиторийдің external/glad папкасында; GLM header-only (жүйеде орнатылған)
add_executable(week10 main.cpp ../external/glad/src/gl.c)
target_include_directories(week10 PRIVATE ../external/glad/include)
target_link_libraries(week10 glfw OpenGL::GL ${CMAKE_DL_LIBS})

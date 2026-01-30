cmake_minimum_required(VERSION 3.20)
project(HelloGlfw LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)

list(APPEND CMAKE_PREFIX_PATH "C:/Dev/st")
find_package(glfw3 REQUIRED)

# ---------- GLAD ----------
add_library(glad STATIC external/src/glad.c)

target_include_directories(glad PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/external/include"  # 如果 glad/include 在这里
    "${CMAKE_PREFIX_PATH}/include"
)

install(TARGETS glad
        ARCHIVE DESTINATION "${CMAKE_PREFIX_PATH}/lib"
        INCLUDES DESTINATION "${CMAKE_PREFIX_PATH}/include")

# ---------- 可执行 ----------
add_executable(hello CMakeProject1/main.cpp)
target_link_libraries(hello PRIVATE glad glfw opengl32)

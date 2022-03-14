cmake_minimum_required(VERSION 3.0.0)
project(window_texture VERSION 0.1.0)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 17)

include(CTest)
enable_testing()

add_executable(window_texture main.cpp d3drender.cpp yuv_file.cpp)

target_link_libraries(window_texture -lstdc++fs)
target_link_libraries(window_texture d3d11.lib D3D11.lib D3DCompiler.lib)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

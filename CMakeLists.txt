#
#cmake .. -DBUILD_SHARED_LIBS=ON // with shared library
#cmake .. -DBUILD_SHARED_LIBS=OFF // with static library
#
# cmake --build . --target clean
# cmake -G "Visual Studio 17 2022" -T ClangCL -A x64 ..
# cmake --build . --config Release
#
#cmake --build . --config Release
#

cmake_minimum_required(VERSION 3.16)
project(miniaudio_player)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(src)
add_subdirectory(player)


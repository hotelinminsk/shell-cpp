cmake_minimum_required(VERSION 3.13)
project(shell-starter-cpp)

# src ve lib altındaki cpp/hpp dosyalarını dahil et
file(GLOB_RECURSE SOURCE_FILES
    src/*.cpp
    src/*.hpp
    lib/**/*.cpp
    lib/**/*.hpp
)

set(CMAKE_CXX_STANDARD 23)

add_executable(shell ${SOURCE_FILES})

target_link_libraries(shell PRIVATE readline)

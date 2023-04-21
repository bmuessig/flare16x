#cmake_minimum_required(VERSION 3.15)
project(flare16x C)

set(CMAKE_C_STANDARD 99)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

#set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
#set(BUILD_SHARED_LIBS OFF)
#set(CMAKE_EXE_LINKER_FLAGS "-static")

add_executable(flare16x main.c bitmap.h bitmap.c palettes.c palettes.h palette_rainbow.c palette_iron.c palette_grayscale.c error.c error.h canvas.c canvas.h ocr.c ocr.h locator.c locator.h thermal.c thermal.h)
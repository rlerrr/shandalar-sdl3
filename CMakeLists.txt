cmake_minimum_required(VERSION 3.24 FATAL_ERROR)

project(shandalar_sdl3 LANGUAGES C CXX)

include(FetchContent)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(SHANDALAR_BUILD_FACEMAKER "Build the SDL3 Facemaker application" ON)
option(SHANDALAR_BUILD_SHANDALAR "Build the SDL3 Shandalar application" OFF)
option(SHANDALAR_BUILD_DECK "Build the SDL3 Deck Builder application" OFF)
option(SHANDALAR_BUILD_TESTS "Build unit tests" ON)
set(SHANDALAR_ASSET_DIR "" CACHE PATH "Directory containing the Magic game data")

if(SHANDALAR_BUILD_TESTS)
  enable_testing()
endif()

set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDL_PTHREADS OFF CACHE BOOL "" FORCE)
set(SDL_AUDIO ${SHANDALAR_BUILD_DECK} CACHE BOOL "" FORCE)
set(SDL_CAMERA OFF CACHE BOOL "" FORCE)
set(SDL_GPU OFF CACHE BOOL "" FORCE)
set(SDL_HAPTIC OFF CACHE BOOL "" FORCE)
set(SDL_JOYSTICK OFF CACHE BOOL "" FORCE)
set(SDL_SENSOR OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  SDL3
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG release-3.4.10
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(SDL3)
if(NOT TARGET SDL3::Headers)
  add_library(SDL3::Headers ALIAS SDL3_Headers)
endif()
if(NOT TARGET SDL3::SDL3)
  add_library(SDL3::SDL3 ALIAS SDL3-static)
endif()

set(SDLTTF_VENDORED ON CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(SDLTTF_HARFBUZZ OFF CACHE BOOL "" FORCE)
set(SDLTTF_PLUTOSVG OFF CACHE BOOL "" FORCE)
set(SDLTTF_SAMPLES OFF CACHE BOOL "" FORCE)
set(SDLTTF_TESTS OFF CACHE BOOL "" FORCE)
set(SDLTTF_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  SDL3_ttf
  GIT_REPOSITORY https://github.com/libsdl-org/SDL_ttf.git
  GIT_TAG release-3.2.2
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(SDL3_ttf)

if(SHANDALAR_BUILD_DECK OR SHANDALAR_BUILD_FACEMAKER OR SHANDALAR_BUILD_SHANDALAR)
  FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.9b
    GIT_SHALLOW TRUE
  )
  FetchContent_MakeAvailable(imgui)
endif()

if(EMSCRIPTEN)
  add_compile_options(-g)
endif()

if(MSVC)
  add_compile_options(/W3 /wd4996)
else()
  add_compile_options(-Wall -Wextra -Wno-unused-parameter -Wno-unused-function)
endif()

if(SHANDALAR_BUILD_FACEMAKER OR SHANDALAR_BUILD_SHANDALAR OR SHANDALAR_BUILD_DECK)
  add_subdirectory(src)
endif()

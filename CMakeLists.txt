cmake_minimum_required(VERSION 3.24 FATAL_ERROR)

project(shandalar_sdl3 LANGUAGES C CXX)

if(WIN32 AND NOT EMSCRIPTEN)
  enable_language(RC)
endif()

include(FetchContent)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(EMSCRIPTEN)
  # Emscripten's libc strlcpy reads past the terminator in word-sized
  # chunks, which ASan catches on short strings. Use SDL's bounded-copy
  # fallback instead, including when reconfiguring an existing build.
  set(LIBC_HAS_STRLCPY OFF)
  # Keep the regular Debug build symbol-enabled without the ASan overhead.
  add_compile_options("$<$<CONFIG:Debug>:-g>")
  # Instrument dependencies too: ASan must be enabled when compiling every
  # library as well as when linking the final WebAssembly application.
  # Unoptimized ASan instrumentation can exceed browser limits on Wasm
  # locals in SDL's macro-heavy software rendering functions.
  add_compile_options("$<$<CONFIG:DebugAsan>:-O1;-g;-fsanitize=address;-fsanitize-recover=address;-mllvm;-asan-instrument-reads=false>")
  # ASan's shadow memory and instrumented globals must fit at link time;
  # runtime memory growth cannot satisfy wasm-ld's initial-memory minimum.
  add_link_options("$<IF:$<OR:$<CONFIG:Debug>,$<CONFIG:DebugAsan>>,-g,--profiling-funcs>"
                   "$<$<CONFIG:DebugAsan>:-O1;-fsanitize=address;-asan-instrument-reads=false;-sASSERTIONS=1;-sINITIAL_MEMORY=256MB>")
endif()

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
if(EMSCRIPTEN)
  # Threadless SDL is only supported by upstream on Emscripten (and N-Gage);
  # native builds (Windows/macOS) require threads. Our own code still avoids
  # spawning threads and pumps UI via Asyncify yields on Emscripten.
  set(SDL_PTHREADS OFF CACHE BOOL "" FORCE)
endif()
if(SHANDALAR_BUILD_DECK OR SHANDALAR_BUILD_SHANDALAR)
  set(SDL_AUDIO ON CACHE BOOL "" FORCE)
else()
  set(SDL_AUDIO OFF CACHE BOOL "" FORCE)
endif()
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
  OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(SDL3)
if(NOT TARGET SDL3::Headers)
  add_library(SDL3::Headers ALIAS SDL3_Headers)
endif()
if(NOT TARGET SDL3::SDL3)
  add_library(SDL3::SDL3 ALIAS SDL3-static)
endif()

if(SHANDALAR_BUILD_SHANDALAR)
  set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
  set(SDLIMAGE_VENDORED ON CACHE BOOL "" FORCE)
  set(SDLIMAGE_DEPS_SHARED OFF CACHE BOOL "" FORCE)
  set(SDLIMAGE_INSTALL OFF CACHE BOOL "" FORCE)
  set(SDLIMAGE_SAMPLES OFF CACHE BOOL "" FORCE)
  set(SDLIMAGE_TESTS OFF CACHE BOOL "" FORCE)
  set(SDLIMAGE_BACKEND_STB OFF CACHE BOOL "" FORCE)
  set(SDLIMAGE_ANI OFF CACHE BOOL "" FORCE)
  set(SDLIMAGE_AVIF OFF CACHE BOOL "" FORCE)
  set(SDLIMAGE_BMP OFF CACHE BOOL "" FORCE)
  set(SDLIMAGE_GIF OFF CACHE BOOL "" FORCE)
  set(SDLIMAGE_JPG OFF CACHE BOOL "" FORCE)
  set(SDLIMAGE_JXL OFF CACHE BOOL "" FORCE)
  set(SDLIMAGE_LBM OFF CACHE BOOL "" FORCE)
  set(SDLIMAGE_PCX OFF CACHE BOOL "" FORCE)
  set(SDLIMAGE_PNG ON CACHE BOOL "" FORCE)
  set(SDLIMAGE_PNG_LIBPNG ON CACHE BOOL "" FORCE)
  set(SDLIMAGE_PNG_SAVE OFF CACHE BOOL "" FORCE)
  set(SDLIMAGE_PNM OFF CACHE BOOL "" FORCE)
  set(SDLIMAGE_QOI OFF CACHE BOOL "" FORCE)
  set(SDLIMAGE_SVG OFF CACHE BOOL "" FORCE)
  set(SDLIMAGE_TGA OFF CACHE BOOL "" FORCE)
  set(SDLIMAGE_TIF OFF CACHE BOOL "" FORCE)
  set(SDLIMAGE_WEBP ON CACHE BOOL "" FORCE)
  set(SDLIMAGE_WEBP_SAVE OFF CACHE BOOL "" FORCE)
  set(SDLIMAGE_WEBP_SHARED OFF CACHE BOOL "" FORCE)
  set(SDLIMAGE_XCF OFF CACHE BOOL "" FORCE)
  set(SDLIMAGE_XPM OFF CACHE BOOL "" FORCE)
  set(SDLIMAGE_XV OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(
    SDL3_image
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_image.git
    GIT_TAG release-3.4.4
    GIT_SHALLOW TRUE
  )
  FetchContent_MakeAvailable(SDL3_image)
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

if(SHANDALAR_BUILD_DECK OR SHANDALAR_BUILD_SHANDALAR)
  FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.9b
    GIT_SHALLOW TRUE
  )
  FetchContent_MakeAvailable(imgui)
endif()

if(MSVC)
  add_compile_options(/W3 /wd4996)
else()
  add_compile_options(-Wall -Wextra -Wno-unused-parameter -Wno-unused-function)
endif()

include(cmake/DownloadAssets.cmake)

if(SHANDALAR_BUILD_FACEMAKER OR SHANDALAR_BUILD_SHANDALAR OR SHANDALAR_BUILD_DECK)
  add_subdirectory(src)
endif()

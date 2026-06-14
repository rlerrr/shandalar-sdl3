add_library(magsnd_static STATIC
  src/sdl.c
)

target_include_directories(magsnd_static PUBLIC
  "${SHANDALAR_SRC_ROOT}"
  "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(magsnd_static PUBLIC shandalar_platform)

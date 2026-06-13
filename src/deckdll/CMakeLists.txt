add_library(deck_core STATIC
  src/card_db.c
  src/deck_file.c
  src/deck_core.c
  src/drm.c
)

target_include_directories(deck_core PUBLIC
  "${SHANDALAR_SRC_ROOT}"
  "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_compile_definitions(deck_core PRIVATE MODERN_FIXES)

target_link_libraries(deck_core PUBLIC
  drawcard_static
  magsnd_static
  shandalar_platform
)

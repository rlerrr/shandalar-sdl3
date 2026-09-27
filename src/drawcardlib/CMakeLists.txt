add_library(drawcard_facemaker STATIC
  src/pcxw.c
  src/pic.c
)

target_include_directories(drawcard_facemaker PUBLIC
  "${SHANDALAR_SRC_ROOT}"
  "${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(drawcard_facemaker PUBLIC
  cardart_facemaker
  rpbits
  shandalar_platform
)
target_compile_definitions(drawcard_facemaker PRIVATE FACEMAKER)

add_library(drawcard_static STATIC
  src/drawing.c
  src/main.c
  src/pcxw.c
  src/pic.c
)

target_include_directories(drawcard_static PUBLIC
  "${SHANDALAR_SRC_ROOT}"
  "${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(drawcard_static PUBLIC
  cardart_static
  rpbits
  shandalar_platform
)
target_compile_definitions(drawcard_static PRIVATE DRAWCARDLIB MODERN_FIXES)

add_library(drawcard_shandalar STATIC
  src/drawing.c
  src/main.c
  src/pcxw.c
  src/pic.c
)

target_include_directories(drawcard_shandalar PUBLIC
  "${SHANDALAR_SRC_ROOT}"
  "${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(drawcard_shandalar PUBLIC
  cardart_static
  rpbits
  shandalar_platform
)
target_compile_definitions(drawcard_shandalar PRIVATE
  DRAWCARDLIB
  MODERN_FIXES
  SHANDALAR
)

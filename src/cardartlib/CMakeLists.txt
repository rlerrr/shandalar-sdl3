add_library(cardart_facemaker STATIC
  src/assert.c
  src/palette_facemaker.c
)

target_include_directories(cardart_facemaker PUBLIC
  "${SHANDALAR_SRC_ROOT}"
  "${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(cardart_facemaker PUBLIC shandalar_platform)
target_compile_definitions(cardart_facemaker PRIVATE FACEMAKER)

add_library(cardart_static STATIC
  src/assert.c
  src/catalog.c
  src/haar.c
  src/huffman.c
  src/main.c
  src/palette.c
)

target_include_directories(cardart_static PUBLIC
  "${SHANDALAR_SRC_ROOT}"
  "${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(cardart_static PUBLIC shandalar_platform)
target_compile_definitions(cardart_static PRIVATE CARDARTLIB MODERN_FIXES SHANDALAR_STATIC_LIB)

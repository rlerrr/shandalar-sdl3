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

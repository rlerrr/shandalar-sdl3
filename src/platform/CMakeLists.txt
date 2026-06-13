add_library(shandalar_platform STATIC
  asset_manager.c
  gdi_compat.c
  legacy_io.c
)

target_include_directories(shandalar_platform PUBLIC
  "${SHANDALAR_SRC_ROOT}"
)
target_link_libraries(shandalar_platform PUBLIC
  SDL3::SDL3
  SDL3_ttf::SDL3_ttf
)

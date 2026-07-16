add_library(statwin_static STATIC
  src/statwin.c
)

target_include_directories(statwin_static PUBLIC
  "${SHANDALAR_SRC_ROOT}"
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(statwin_static PUBLIC
  SDL3::SDL3
  SDL3_image::SDL3_image
)

target_link_libraries(statwin_static PRIVATE
  magsnd_static
  shandalar_platform
)

if(SHANDALAR_BUILD_TESTS)
  add_executable(statwin_asset_test
    tests/statwin_asset_test.c
  )
  target_link_libraries(statwin_asset_test PRIVATE
    SDL3::SDL3
    SDL3_image::SDL3_image
  )
  if(SHANDALAR_ASSET_DIR)
    add_test(
      NAME statwin_assets
      COMMAND statwin_asset_test "${SHANDALAR_ASSET_DIR}/Statwin_SDL"
    )
  endif()
endif()

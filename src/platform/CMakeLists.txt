add_library(shandalar_platform STATIC
  asset_manager.c
  file_io.c
  gdi_compat.c
  legacy_io.c
  dictionary.c
  iniparser.c
  vfw_compat.c
  winbase_compat.c
  wincontrols_compat.c
  windialog_compat.c
  winmenu_compat.c
  winuser_compat.c
  winuser_runtime.c
  winuser_sdl_compositor.c
  winreg_compat.c
  winscroll_compat.c
)

target_include_directories(shandalar_platform PUBLIC
  "${SHANDALAR_SRC_ROOT}"
)
target_link_libraries(shandalar_platform PUBLIC
  SDL3::SDL3
  SDL3_ttf::SDL3_ttf
)

if(SHANDALAR_BUILD_DECK OR SHANDALAR_BUILD_SHANDALAR)
  add_library(legacy_imgui STATIC
    "${imgui_SOURCE_DIR}/imgui.cpp"
    "${imgui_SOURCE_DIR}/imgui_draw.cpp"
    "${imgui_SOURCE_DIR}/imgui_tables.cpp"
    "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp"
  )
  target_include_directories(legacy_imgui PUBLIC
    "${imgui_SOURCE_DIR}"
    "${imgui_SOURCE_DIR}/backends"
  )
  target_link_libraries(legacy_imgui PUBLIC SDL3::SDL3)

  add_library(legacy_win32_imgui_bridge STATIC
    winuser_imgui_bridge.cpp
  )
  target_include_directories(legacy_win32_imgui_bridge PUBLIC
    "${SHANDALAR_SRC_ROOT}"
    "${imgui_SOURCE_DIR}"
  )
  target_link_libraries(legacy_win32_imgui_bridge PUBLIC
    legacy_imgui
    shandalar_platform
  )
endif()

if(SHANDALAR_BUILD_TESTS)
  add_executable(winuser_compat_test tests/winuser_compat_test.c)
  target_link_libraries(winuser_compat_test PRIVATE shandalar_platform)
  add_test(NAME winuser_compat_test COMMAND winuser_compat_test)
endif()

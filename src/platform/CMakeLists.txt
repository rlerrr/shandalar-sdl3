add_library(shandalar_platform STATIC
  asset_manager.c
  gdi_compat.c
  legacy_io.c
  winbase_compat.c
  wincontrols_compat.c
  windialog_compat.c
  winmenu_compat.c
  winuser_compat.c
  winuser_runtime.c
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

if(SHANDALAR_BUILD_TESTS)
  add_executable(winuser_compat_test tests/winuser_compat_test.c)
  target_link_libraries(winuser_compat_test PRIVATE shandalar_platform)
  add_test(NAME winuser_compat_test COMMAND winuser_compat_test)
endif()

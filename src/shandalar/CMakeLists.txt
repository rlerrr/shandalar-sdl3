add_executable(shandalar
  src/sdl_main.c
  src/runtime.c
  src/opt57_shandalar_core.c
  src/openingMenu.c
  src/iniFile.c
  ../magic/src/shandalar_opening_globals.c
)

target_compile_definitions(shandalar PRIVATE MODERN_FIXES SHANDALAR)
target_include_directories(shandalar PRIVATE
  "${SHANDALAR_SRC_ROOT}"
  "${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(shandalar PRIVATE
  facemaker_core
  SDL3::SDL3
  SDL3_ttf::SDL3_ttf
)

if(EMSCRIPTEN)
  set_target_properties(shandalar PROPERTIES SUFFIX ".html")
  target_link_options(shandalar PRIVATE
    "-sALLOW_MEMORY_GROWTH=1"
    "-sFORCE_FILESYSTEM=1"
    "-sMAXIMUM_MEMORY=1GB"
    "-sASYNCIFY=1"
  )
  if(SHANDALAR_ASSET_DIR)
    target_link_options(shandalar PRIVATE
      "SHELL:--preload-file ${SHANDALAR_ASSET_DIR}@/assets"
      "SHELL:--exclude-file ${SHANDALAR_ASSET_DIR}/Tutorial"
    )
  endif()
endif()

install(TARGETS shandalar RUNTIME DESTINATION .)

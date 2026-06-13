add_library(facemaker_core STATIC
  src/core.c
  src/editor.c
  src/face_assets.c
  src/fileio.c
  src/lib.c
)

target_compile_definitions(facemaker_core PRIVATE FACEMAKER MODERN_FIXES)
target_include_directories(facemaker_core PUBLIC
  "${SHANDALAR_SRC_ROOT}"
  "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_link_libraries(facemaker_core PUBLIC
  drawcard_facemaker
  cardart_facemaker
  rpbits
  shandalar_platform
)

add_executable(facemaker src/sdl_main.c)
target_link_libraries(facemaker PRIVATE
  facemaker_core
  SDL3::SDL3
  SDL3_ttf::SDL3_ttf
)

if(EMSCRIPTEN)
  set_target_properties(facemaker PROPERTIES SUFFIX ".html")
  target_link_options(facemaker PRIVATE
    "-sALLOW_MEMORY_GROWTH=1"
    "-sFORCE_FILESYSTEM=1"
    "-sMAXIMUM_MEMORY=1GB"
  )
  if(SHANDALAR_ASSET_DIR)
    target_link_options(facemaker PRIVATE
      "SHELL:--preload-file ${SHANDALAR_ASSET_DIR}/FaceArt@/assets/FaceArt"
      "SHELL:--preload-file ${SHANDALAR_ASSET_DIR}/FaceData.txt@/assets/FaceData.txt"
      "SHELL:--preload-file ${SHANDALAR_ASSET_DIR}/FaceMostRecent.txt@/assets/FaceMostRecent.txt"
      "SHELL:--preload-file ${SHANDALAR_ASSET_DIR}/TT0530M_.TTF@/assets/TT0530M_.TTF"
    )
  endif()
endif()

install(TARGETS facemaker facemaker_core
  RUNTIME DESTINATION .
  ARCHIVE DESTINATION lib
)

if(SHANDALAR_BUILD_TESTS AND NOT EMSCRIPTEN)
  add_executable(facemaker_core_smoke tests/core_smoke.c)
  target_link_libraries(facemaker_core_smoke PRIVATE facemaker_core)
  add_test(
    NAME facemaker_core_smoke
    COMMAND facemaker_core_smoke "${SHANDALAR_ASSET_DIR}"
  )
endif()

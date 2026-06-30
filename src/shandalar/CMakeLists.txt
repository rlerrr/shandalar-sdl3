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
  facemaker_legacy_ui
  facemaker_win32_imgui_bridge
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
    "-sFETCH=1"
    "-gsource-map"
  )
  if(SHANDALAR_ASSET_DIR)
    function(shandalar_preload_asset asset_source asset_dest)
      target_link_options(shandalar PRIVATE
        "SHELL:--preload-file ${SHANDALAR_ASSET_DIR}/${asset_source}@/assets/${asset_dest}"
      )
    endfunction()

    set(shandalar_opening_preload_assets
      advButtons.txt|advButtons.txt
      MENUBAK.PIC|menubak.pic
      BEGIN.SPR|begin.spr
      MENU2.PIC|menu2.pic
      menu2-norm.pic|menu2-norm.pic
      MENU2-HI.PIC|menu2-hi.pic
      MENU3.PIC|menu3.pic
      menu3-but1.pic|menu3-but1.pic
      MENU3BUT.PIC|menu3but.pic
      NAMEPICK.PIC|namepick.pic
      AdvStrings.txt|ADVstrings.txt
      ADVFAC64.PIC|advfac64.pic
      pedstls.pic|pedstls.pic
      MENOPT.PIC|menopt.pic
      OPTBOX.PIC|optbox.pic
      FaceButtons.txt|FaceButtons.txt
      FaceData.txt|FaceData.txt
      FaceMostRecent.txt|FaceMostRecent.txt
      TT0127M_.TTF|TT0127M_.TTF
      TT0298M_.TTF|TT0298M_.TTF
      TT0300M_.TTF|TT0300M_.TTF
      TT0530M_.TTF|TT0530M_.TTF
    )
    foreach(shandalar_preload_entry IN LISTS shandalar_opening_preload_assets)
      string(REPLACE "|" ";" shandalar_preload_pair "${shandalar_preload_entry}")
      list(GET shandalar_preload_pair 0 shandalar_preload_source)
      list(GET shandalar_preload_pair 1 shandalar_preload_dest)
      shandalar_preload_asset("${shandalar_preload_source}" "${shandalar_preload_dest}")
    endforeach()
    target_link_options(shandalar PRIVATE
      "SHELL:--preload-file ${SHANDALAR_ASSET_DIR}/FONTS.CV@/assets/fonts.cv"
    )
  endif()
endif()

install(TARGETS shandalar RUNTIME DESTINATION .)

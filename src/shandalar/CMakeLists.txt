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
      MAGIC3.map|magic3.map
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
      saveDescs|saveDescs
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

    foreach(shandalar_save_slot RANGE 4 13)
      if(shandalar_save_slot LESS 10)
        set(shandalar_save_suffix "${shandalar_save_slot}")
      elseif(shandalar_save_slot EQUAL 10)
        set(shandalar_save_suffix "a")
      elseif(shandalar_save_slot EQUAL 11)
        set(shandalar_save_suffix "b")
      elseif(shandalar_save_slot EQUAL 12)
        set(shandalar_save_suffix "c")
      else()
        set(shandalar_save_suffix "d")
      endif()
      set(shandalar_save_map "magic${shandalar_save_suffix}.map")
      if(EXISTS "${SHANDALAR_ASSET_DIR}/${shandalar_save_map}")
        shandalar_preload_asset("${shandalar_save_map}" "${shandalar_save_map}")
      endif()
    endforeach()
    target_link_options(shandalar PRIVATE
      "SHELL:--preload-file ${SHANDALAR_ASSET_DIR}/FONTS.CV@/assets/fonts.cv"
    )
  endif()
endif()

install(TARGETS shandalar RUNTIME DESTINATION .)

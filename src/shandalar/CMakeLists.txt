add_executable(shandalar
  src/sdl_main.c
  src/shandalar.c
  src/opt57_shandalar_core.c
  src/opt57_shandalar_only.c
  src/opt57_shandalar_ui_fx.c
  src/openingMenu.c
  src/adventureWorldUi.c
  src/world_lair_monster_slots.c
  src/world_lair_monster_encounter.c
  src/visitLocation.c
  src/dungeonEncounter.c
  src/adventureStats.c
  src/mapScreen.c
  src/cityInfoScreen.c
  src/dungeonCluesScreen.c
  src/hintText.c
  src/adventureListPicker.c
  src/iniFile.c
  src/magic_runtime_stubs.c
  src/shandalar_global_strings.c
  ../magic/src/global_duel_ui_ids.c
  ../magic/src/global_other.c
  ../magic/src/global_state.c
  ../magic/src/global_strings.c
  ../magic/src/shared_startup.c
  ../magic/src/dummycards.c
  ../magic/src/carddata.c
)

target_compile_definitions(shandalar PRIVATE
  MODERN_FIXES
  SHANDALAR
  SHANDALAR_SDL_WIN32_COMPAT
  LoadPcxIntoPageNoPalette=ShandalarLoadPcxIntoPageNoPalette
)
if(EMSCRIPTEN)
  target_compile_options(shandalar PRIVATE -include platform/legacy_io.h)
endif()
target_include_directories(shandalar PRIVATE
  "${SHANDALAR_SRC_ROOT}"
  "${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(shandalar PRIVATE
  facemaker_legacy_ui
  facemaker_win32_imgui_bridge
  deck_legacy_ui
  cardart_static
  drawcard_shandalar
  magsnd_static
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
    "-lidbfs.js"
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

include("${SHANDALAR_SRC_ROOT}/magic/duel_sources.cmake")

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
  src/magic_runtime.c
  src/shandalar_global_strings.c
  ${SHANDALAR_MAGIC_DUEL_SOURCES}
  ../magic/src/global_duel_ui_ids.c
  ../magic/src/global_other.c
  ../magic/src/global_state.c
  ../magic/src/global_strings.c
  ../magic/src/shared_startup.c
  ../magic/src/carddata.c
)

target_compile_definitions(shandalar PRIVATE
  MODERN_FIXES
  SHANDALAR
  SHANDALAR_SDL_WIN32_COMPAT
  LoadPcxIntoPageNoPalette=ShandalarLoadPcxIntoPageNoPalette
)
if(EMSCRIPTEN)
  target_compile_options(shandalar PRIVATE
    -include platform/legacy_io.h
  )
endif()
target_include_directories(shandalar PRIVATE
  "${SHANDALAR_SRC_ROOT}"
  "${CMAKE_CURRENT_SOURCE_DIR}"
)
target_include_directories(shandalar BEFORE PRIVATE
  "${SHANDALAR_SRC_ROOT}/magic/compat"
)
target_link_libraries(shandalar PRIVATE
  facemaker_legacy_ui
  deck_imgui_interaction
  legacy_win32_imgui_bridge
  deck_legacy_ui
  cardart_static
  drawcard_shandalar
  magsnd_static
  statwin_static
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
    "-sASYNCIFY_STACK_SIZE=16384"
    "-sEXIT_RUNTIME=1"
    "-sFETCH=1"
    "--profiling-funcs"
    "-lidbfs.js"
  )
  if(SHANDALAR_ASSET_DIR)
    set(shandalar_opening_preload_assets
      advButtons.txt
      MENUBAK.PIC
      AdvStrings.txt
      FaceButtons.txt
      FaceData.txt   
      uistrings.txt
      advblocks.txt
      hints.txt
      concise.csv
      TT0127M_.TTF
      TT0298M_.TTF
      TT0300M_.TTF
      TT0530M_.TTF
      TT0085M_.TTF
      TT0299M_.TTF
      Magis___.TTF
      Magim___.TTF
      fonts.cv
      DuelArt/Duel.dat
      duelpalall.tr
      duel.plogpal
      cards.dat
      cardart/manasymbols.pic
      cardart/cardsets.pic
      cardart/cardback.pic
      sound/kwalkl.wav
      sound/kwalkr.wav
      sound/bwalkl.wav
      sound/bwalkr.wav
      sound/gwalkl.wav
      sound/gwalkr.wav
      sound/rwalkl.wav
      sound/rwalkr.wav
      sound/wwalkl.wav
      sound/wwalkr.wav
      sound/kbird1.wav
      sound/bbird1.wav
      sound/gbird1.wav
      sound/rbird1.wav
      sound/wbird1.wav
      todpal.tr
      advfac64.pic
      dbox.spr
      begin.spr
    )
    download_assets(${shandalar_opening_preload_assets})
    foreach(shandalar_preload_entry IN LISTS shandalar_opening_preload_assets)
      target_link_options(shandalar PRIVATE
        "SHELL:--preload-file ${SHANDALAR_ASSET_DIR}/${shandalar_preload_entry}@/assets/${shandalar_preload_entry}"
      )
    endforeach()
  endif()
endif()

install(TARGETS shandalar RUNTIME DESTINATION .)

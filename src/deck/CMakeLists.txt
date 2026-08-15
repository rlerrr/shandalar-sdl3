if(CMAKE_SIZEOF_VOID_P EQUAL 4)
  add_executable(deck
    src/legacy_sdl_main.c
  )
  target_include_directories(deck PRIVATE
    "${SHANDALAR_SRC_ROOT}"
  )
  target_compile_definitions(deck PRIVATE SHANDALAR_SDL_WIN32_COMPAT)
  target_link_libraries(deck PRIVATE
    deck_legacy_ui
    deck_imgui_interaction
    legacy_win32_imgui_bridge
    SDL3::SDL3
  )
  if(EMSCRIPTEN)
    set_target_properties(deck PROPERTIES SUFFIX ".html")
    target_link_options(deck PRIVATE
      "-sALLOW_MEMORY_GROWTH=1"
      "-sFORCE_FILESYSTEM=1"
      "-sASYNCIFY=1"
      "-sASYNCIFY_STACK_SIZE=16384"
      "-sEXIT_RUNTIME=1"
      "-sFETCH=1"
      "--profiling-funcs"
    )
    if(SHANDALAR_ASSET_DIR)
      set(deck_preload_assets
        MENUBAK.PIC
        cards.dat
        Deckbuilder.csv
        MENUS.TXT
        menusx1.txt
        CUECARDS.TXT
        UIStrings.txt
        MP_UIStrings.txt
        duelpalall.tr
        duel.plogpal
        TT0085M_.TTF
        TT0127M_.TTF
        TT0298M_.TTF
        TT0299M_.TTF
        TT0300M_.TTF
        TT0530M_.TTF
        Magis___.TTF
        Magim___.TTF
        fonts.cv
        DuelArt/Duel.dat
        DuelSounds/DRAW.WAV
        DuelSounds/DISCARD.WAV
        DuelSounds/BUTTON.WAV
        DuelSounds/CANCEL.WAV
      )
      download_assets(${deck_preload_assets})
      foreach(deck_preload_asset IN LISTS deck_preload_assets)
        target_link_options(deck PRIVATE
          "SHELL:--preload-file ${SHANDALAR_ASSET_DIR}/${deck_preload_asset}@/assets/${deck_preload_asset}"
        )
      endforeach()
    endif()
  endif()
else()
  message(FATAL_ERROR "The deck target now uses the legacy deck builder and requires a 32-bit build.")
endif()

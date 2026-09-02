if(CMAKE_SIZEOF_VOID_P EQUAL 4)
  add_executable(deck
    src/legacy_sdl_main.c
  )
  if(WIN32 AND NOT EMSCRIPTEN)
    set(deck_icon_resource "${CMAKE_CURRENT_BINARY_DIR}/deck_icon.rc")
    file(GENERATE OUTPUT "${deck_icon_resource}"
      CONTENT "1 ICON \"${CMAKE_CURRENT_SOURCE_DIR}/deck.ico\"\n"
    )
    set(deck_icon_object "${CMAKE_CURRENT_BINARY_DIR}/deck_icon.res")
    add_custom_command(OUTPUT "${deck_icon_object}"
      COMMAND "${CMAKE_RC_COMPILER}" /nologo /fo "${deck_icon_object}" "${deck_icon_resource}"
      DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/deck.ico"
      VERBATIM
    )
    add_custom_target(deck_icon DEPENDS "${deck_icon_object}")
    add_dependencies(deck deck_icon)
    set_source_files_properties("${deck_icon_object}" PROPERTIES GENERATED TRUE EXTERNAL_OBJECT TRUE)
    target_sources(deck PRIVATE "${deck_icon_object}")
  endif()
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
      "-lidbfs.js"
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

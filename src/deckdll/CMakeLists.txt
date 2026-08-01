if(CMAKE_SIZEOF_VOID_P EQUAL 4)
  add_library(deck_legacy_ui STATIC
    src/buttonclass.c
    src/card_db.c
    src/checks.c
    src/deckdll.c
    src/deckstats.c
    src/decksurface.c
    src/dialog_resources.c
    src/drm.c
    src/filtermenus.c
    src/magsnd_sdl_adapter.c
    src/shared_resources.c
  )

  target_include_directories(deck_legacy_ui PUBLIC
    "${SHANDALAR_SRC_ROOT}"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
  )
  target_compile_definitions(deck_legacy_ui PRIVATE
    DECKDLL
    MODERN_FIXES
    SHANDALAR_SDL_WIN32_COMPAT
    global_hinstance=deckdll_global_hinstance
    LoadFontFromIni=deckdll_LoadFontFromIni
    load_text=deckdll_load_text
    deck=deckdll_deck
    Gold=deckdll_Gold
    Scards=deckdll_Scards
    _PlayerFace=deckdll_PlayerFace
    _OpponFace=deckdll_OpponFace
    global_duelart_path=deckdll_global_duelart_path
    create_fonts=deckdll_create_fonts
    destroy_create_fonts_resources=deckdll_destroy_create_fonts_resources
    global_duel_dat_path=deckdll_global_duel_dat_path
    global_cardart_path=deckdll_global_cardart_path
  )
  target_link_libraries(deck_legacy_ui PUBLIC
    drawcard_static
    magsnd_static
    shandalar_platform
  )

  add_library(deck_imgui_interaction STATIC
    "${SHANDALAR_SRC_ROOT}/deck/src/deck_imgui_interaction.cpp"
  )
  target_include_directories(deck_imgui_interaction PUBLIC
    "${SHANDALAR_SRC_ROOT}"
  )
  target_link_libraries(deck_imgui_interaction PUBLIC
    legacy_win32_imgui_bridge
  )
endif()

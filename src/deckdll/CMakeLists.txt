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
  )
  target_link_libraries(deck_legacy_ui PUBLIC
    drawcard_static
    magsnd_static
    shandalar_platform
  )
endif()

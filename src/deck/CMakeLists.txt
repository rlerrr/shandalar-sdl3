add_library(deck_imgui STATIC
  "${imgui_SOURCE_DIR}/imgui.cpp"
  "${imgui_SOURCE_DIR}/imgui_draw.cpp"
  "${imgui_SOURCE_DIR}/imgui_tables.cpp"
  "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
  "${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp"
  "${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp"
)

target_include_directories(deck_imgui PUBLIC
  "${imgui_SOURCE_DIR}"
  "${imgui_SOURCE_DIR}/backends"
)

target_link_libraries(deck_imgui PUBLIC SDL3::SDL3)

if(CMAKE_SIZEOF_VOID_P EQUAL 4)
  add_library(deck_win32_imgui_bridge STATIC
    src/winuser_imgui_bridge.cpp
  )
  target_include_directories(deck_win32_imgui_bridge PUBLIC
    "${SHANDALAR_SRC_ROOT}"
    "${imgui_SOURCE_DIR}"
  )
  target_link_libraries(deck_win32_imgui_bridge PUBLIC
    deck_imgui
    shandalar_platform
  )

  add_executable(deck
    src/legacy_sdl_main.c
  )
  target_include_directories(deck PRIVATE
    "${SHANDALAR_SRC_ROOT}"
  )
  target_compile_definitions(deck PRIVATE SHANDALAR_SDL_WIN32_COMPAT)
  target_link_libraries(deck PRIVATE
    deck_legacy_ui
    deck_win32_imgui_bridge
    SDL3::SDL3
  )
  if(EMSCRIPTEN)
    set_target_properties(deck PROPERTIES SUFFIX ".html")
    target_link_options(deck PRIVATE
      "-sALLOW_MEMORY_GROWTH=1"
      "-sFORCE_FILESYSTEM=1"
      "-sASYNCIFY=1"
    )
    if(SHANDALAR_ASSET_DIR)
      target_link_options(deck PRIVATE
        "SHELL:--preload-file ${SHANDALAR_ASSET_DIR}@/assets"
      )
    endif()
  endif()
else()
  message(FATAL_ERROR "The deck target now uses the legacy deck builder and requires a 32-bit build.")
endif()

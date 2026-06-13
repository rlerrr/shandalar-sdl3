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

add_executable(deck
  src/sdl_main.c
  src/deck_imgui.cpp
)

target_include_directories(deck PRIVATE
  "${SHANDALAR_SRC_ROOT}"
  "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(deck PRIVATE
  deck_core
  deck_imgui
  SDL3::SDL3
)

if(EMSCRIPTEN)
  target_link_options(deck PRIVATE
    "-sFETCH=1"
    "-sALLOW_MEMORY_GROWTH=1"
  )
endif()

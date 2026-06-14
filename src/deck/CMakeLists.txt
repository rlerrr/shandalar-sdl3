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
  if(SHANDALAR_ASSET_DIR)
    set(DECK_CARD_BACKGROUND_ASSETS
      CARDBK_Green.pic
      CARDBK_White.pic
      CARDBK_Blue.pic
      CARDBK_Black.pic
      CARDBK_Red.pic
      CARDBK_Gold.pic
      CARDBK_Artifact.pic
      CARDBK_GreenLand.pic
      CARDBK_WhiteLand.pic
      CARDBK_BlueLand.pic
      CARDBK_BlackLand.pic
      CARDBK_RedLand.pic
      CARDBK_DarklandsLand.pic
      CARDBK_FallenEmpiresLand.pic
      CARDBK_AntiquitiesLand.pic
      CARDBK_LegendsLand.pic
      CARDBK_ArabianNightsLand.pic
      CARDBK_Special.pic
      CARDBK_IceAgeLand.pic
      CARDBK_Legends.pic
    )
    foreach(asset IN LISTS DECK_CARD_BACKGROUND_ASSETS)
      target_link_options(deck PRIVATE
        "SHELL:--embed-file ${SHANDALAR_ASSET_DIR}/CardArt/${asset}@/assets/CardArt/${asset}"
      )
    endforeach()
    set(DECK_SOUND_ASSETS
      DRAW.WAV
      DISCARD.WAV
    )
    foreach(asset IN LISTS DECK_SOUND_ASSETS)
      target_link_options(deck PRIVATE
        "SHELL:--embed-file ${SHANDALAR_ASSET_DIR}/DuelSounds/${asset}@/assets/DuelSounds/${asset}"
      )
    endforeach()
  endif()
endif()

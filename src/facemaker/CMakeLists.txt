if(SHANDALAR_BUILD_FACEMAKER OR SHANDALAR_BUILD_SHANDALAR)
  add_library(facemaker_imgui STATIC
    "${imgui_SOURCE_DIR}/imgui.cpp"
    "${imgui_SOURCE_DIR}/imgui_draw.cpp"
    "${imgui_SOURCE_DIR}/imgui_tables.cpp"
    "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp"
  )
  target_include_directories(facemaker_imgui PUBLIC
    "${imgui_SOURCE_DIR}"
    "${imgui_SOURCE_DIR}/backends"
  )
  target_link_libraries(facemaker_imgui PUBLIC SDL3::SDL3)

  add_library(facemaker_win32_imgui_bridge STATIC
    ../deck/src/winuser_imgui_bridge.cpp
  )
  target_include_directories(facemaker_win32_imgui_bridge PUBLIC
    "${SHANDALAR_SRC_ROOT}"
    "${imgui_SOURCE_DIR}"
  )
  target_link_libraries(facemaker_win32_imgui_bridge PUBLIC
    facemaker_imgui
    shandalar_platform
  )

  add_library(facemaker_legacy_ui STATIC
    src/main.c
    src/face_assets.c
    src/fileio.c
    src/lib.c
    src/sprite.c
    src/text.c
  )
  target_compile_definitions(facemaker_legacy_ui PRIVATE
    FACEMAKER
    MODERN_FIXES
    SHANDALAR_SDL_WIN32_COMPAT
    WinMain=FacemakerWinMain
    ConsumeUiTickCount=FacemakerConsumeUiTickCount
    MainWindowProc=FacemakerMainWindowProc
    ChangeDisplayResolution=FacemakerChangeDisplayResolution
    RestoreDisplayResolution=FacemakerRestoreDisplayResolution
    DrawTextAt=FacemakerDrawTextAt
    g_app_instance=facemaker_g_app_instance
    g_palette_class_registered=facemaker_g_palette_class_registered
    g_mouse_x=facemaker_g_mouse_x
    g_mouse_y=facemaker_g_mouse_y
    g_ui_tick_count=facemaker_g_ui_tick_count
    ScaleUiCoordinate=FacemakerScaleUiCoordinate
  )
  target_include_directories(facemaker_legacy_ui PUBLIC
    "${SHANDALAR_SRC_ROOT}"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
  )
  target_link_libraries(facemaker_legacy_ui PUBLIC
    drawcard_facemaker
    cardart_facemaker
    rpbits
    shandalar_platform
  )
  if(SHANDALAR_BUILD_SHANDALAR)
    target_compile_definitions(facemaker_legacy_ui PRIVATE
      SHANDALAR_EMBEDDED_FACEMAKER
      SHANDALAR_FONTSLOT_LAYOUT
    )
  endif()
endif()

if(SHANDALAR_BUILD_FACEMAKER)
  add_executable(facemaker src/legacy_sdl_main.c)
  target_link_libraries(facemaker PRIVATE
    facemaker_legacy_ui
    facemaker_win32_imgui_bridge
    SDL3::SDL3
  )

  if(EMSCRIPTEN)
    set_target_properties(facemaker PROPERTIES SUFFIX ".html")
    target_link_options(facemaker PRIVATE
      "-sALLOW_MEMORY_GROWTH=1"
      "-sFORCE_FILESYSTEM=1"
      "-sMAXIMUM_MEMORY=1GB"
      "-sASYNCIFY=1"
      "-sFETCH=1"
      "--profiling-funcs"
      "-lidbfs.js"
    )
    if(SHANDALAR_ASSET_DIR)
      set(facemaker_preload_assets
        FaceButtons.txt
        FaceData.txt
        fonts.cv
        TT0085M_.TTF
        TT0127M_.TTF
        TT0298M_.TTF
        TT0299M_.TTF
        TT0300M_.TTF
        TT0530M_.TTF

      )
      download_assets(${facemaker_preload_assets})
      foreach(entry IN LISTS facemaker_preload_assets)
        target_link_options(facemaker PRIVATE
          "SHELL:--preload-file ${SHANDALAR_ASSET_DIR}/${entry}@/assets/${entry}"
        )
      endforeach()
    endif()
  endif()

  install(TARGETS facemaker
    RUNTIME DESTINATION .
  )
endif()

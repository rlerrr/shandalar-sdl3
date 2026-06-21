set(SHANDALAR_SRC_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")

add_subdirectory(platform)
add_subdirectory(rpbits)
add_subdirectory(cardartlib)
add_subdirectory(drawcardlib)

if(SHANDALAR_BUILD_FACEMAKER)
  add_subdirectory(facemaker)
endif()

if(SHANDALAR_BUILD_SHANDALAR)
  if(NOT SHANDALAR_BUILD_FACEMAKER)
    add_subdirectory(facemaker)
  endif()
  add_subdirectory(shandalar)
endif()

if(SHANDALAR_BUILD_DECK)
  add_subdirectory(magsnd)
  add_subdirectory(deckdll)
  add_subdirectory(deck)
endif()

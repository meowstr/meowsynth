cmake_minimum_required( VERSION 3.16 )

project( meowsynth )

set( CMAKE_EXPORT_COMPILE_COMMANDS ON )

set( GAME_SOURCES
  # includes
  src/audio.hpp
  src/hardware.hpp
  src/logging.hpp
  src/render.hpp
  src/state.hpp
  src/pa_ringbuffer.c

  # sources
  src/pa_audio.cpp
  src/logging.cpp
  src/main.cpp
  src/render.cpp
  src/state.cpp
  src/pa_ringbuffer.h
  src/pa_memorybarrier.h
)

# common libs
add_library( glad libs/glad/src/glad.c )
target_include_directories( glad PUBLIC libs/glad/include )

#
# linux build 
#
if ( (NOT DEFINED EMSCRIPTEN) AND UNIX )

  # pull libraries from the system
  find_package( PkgConfig REQUIRED )
  pkg_check_modules( GLFW REQUIRED IMPORTED_TARGET glfw3 )
  pkg_check_modules( PORTAUDIO REQUIRED IMPORTED_TARGET portaudio-2.0 )
  pkg_check_modules( PORTMIDI REQUIRED IMPORTED_TARGET portmidi )
  add_executable( app ${GAME_SOURCES} src/platform/desktop.cpp )
  target_link_libraries( app PRIVATE glad PkgConfig::GLFW PkgConfig::PORTAUDIO PkgConfig::PORTMIDI )
  add_custom_target( run COMMAND app DEPENDS app WORKING_DIRECTORY ${CMAKE_PROJECT_DIR} )

endif()

#
# windows build
#
if ( (NOT DEFINED EMSCRIPTEN) AND WIN32 )

  # pull prebuilt libraries
  add_library( glfw STATIC IMPORTED )
  set_target_properties( glfw PROPERTIES IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/libs/glfw/lib/glfw3.lib )
  target_include_directories( glfw INTERFACE libs/glfw/include )

  add_library( portaudio STATIC IMPORTED )
  set_target_properties( portaudio PROPERTIES IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/libs/portaudio/lib/portaudio.lib )
  target_include_directories( portaudio INTERFACE libs/portaudio/include )

  add_library( portmidi SHARED IMPORTED )
  set_target_properties( portmidi PROPERTIES IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/libs/portmidi/lib/portmidi.dll )
  set_target_properties( portmidi PROPERTIES IMPORTED_IMPLIB ${PROJECT_SOURCE_DIR}/libs/portmidi/lib/portmidi.lib )
  target_include_directories( portmidi INTERFACE libs/portmidi/include )

  add_executable( app ${GAME_SOURCES} src/platform/desktop.cpp )
  set_target_properties(app PROPERTIES WIN32_EXECUTABLE $<CONFIG:Release> )
  target_link_libraries( app PRIVATE glad glfw portaudio portmidi )
endif()

# common build flags
target_include_directories( app PRIVATE src )
target_compile_features( app PRIVATE cxx_std_20 )
target_compile_definitions( app PRIVATE "RELEASE=$<CONFIG:Release>" )


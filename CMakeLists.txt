cmake_minimum_required(VERSION 3.23)
project(funny-keyboard LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)

option(BUILD_SHARED_LIBS "Build using shared libraries" ON)

include(cmake/CPM.cmake)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Force static linking for dependencies" FORCE)

CPMAddPackage(
  NAME raylib
  GITHUB_REPOSITORY raysan5/raylib
  GIT_TAG 5.5
  OPTIONS
    "BUILD_EXAMPLES OFF"
    "BUILD_GAMES OFF"
    "SUPPORT_WINMM OFF"
)

CPMAddPackage(
  NAME nlohmann_json
  VERSION 3.11.3
  GITHUB_REPOSITORY nlohmann/json
  OPTIONS
    "JSON_BuildTests OFF"
)

CPMAddPackage(
  NAME libwebp
  VERSION 1.3.2
  GITHUB_REPOSITORY webmproject/libwebp
  OPTIONS
    "WEBP_BUILD_ANIM_UTILS OFF"
    "WEBP_BUILD_CWEBP OFF"
    "WEBP_BUILD_DWEBP OFF"
    "WEBP_BUILD_GIF2WEBP OFF"
    "WEBP_BUILD_IMG2WEBP OFF"
    "WEBP_BUILD_VWEBP OFF"
    "WEBP_BUILD_WEBPINFO OFF"
    "WEBP_BUILD_WEBPMUX OFF"
    "WEBP_BUILD_EXTRAS OFF"
)

add_compile_definitions(PLATFORM_DESKTOP)

add_executable(funny-keyboard WIN32 src/main.cpp)

if(WIN32)
  set_target_properties(funny-keyboard PROPERTIES
    WIN32_EXECUTABLE $<IF:$<CONFIG:Release>,TRUE,FALSE>
  )
  target_link_options(funny-keyboard PRIVATE
    $<$<CONFIG:Release>:-Xlinker>
    $<$<CONFIG:Release>:/ENTRY:mainCRTStartup>
  )
endif()

if(TARGET raylib)
  target_link_libraries(funny-keyboard PRIVATE raylib)
endif()

if(TARGET nlohmann_json)
  target_link_libraries(funny-keyboard PRIVATE nlohmann_json::nlohmann_json)
endif()

if(TARGET webp OR TARGET webpdemux)
  if(TARGET webpdemux)
    target_link_libraries(funny-keyboard PRIVATE webpdemux webp)
  else()
    target_link_libraries(funny-keyboard PRIVATE webp)
  endif()
endif()

target_compile_definitions(funny-keyboard PRIVATE
  $<$<CONFIG:Debug>:DEBUG>
  $<$<CONFIG:Release>:RELEASE>
)
 
if(MSVC)
  target_compile_options(funny-keyboard PRIVATE /MP)
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

cmake_minimum_required(VERSION 3.22)
project(liberty_driver_proxy LANGUAGES C CXX)

if(NOT ANDROID OR NOT ANDROID_ABI STREQUAL "arm64-v8a")
  message(FATAL_ERROR "This proxy supports Android arm64-v8a only")
endif()
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
add_link_options("-Wl,-z,max-page-size=16384")
add_subdirectory(vendor/libadrenotools)

add_library(vulkan SHARED driver_proxy.cpp)
target_compile_definitions(vulkan PRIVATE VK_NO_PROTOTYPES)
target_compile_options(vulkan PRIVATE -Wall -Wextra -Werror -fvisibility=hidden)
# Pixel/modern Android builds must not link the legacy private-linker
# constructor at all. Keep custom drivers an explicit legacy build option.
option(LIBERTY_ANDROID_SYSTEM_VULKAN_ONLY "Build without private linker/custom driver support" ON)
target_link_libraries(vulkan PRIVATE log dl)
if(LIBERTY_ANDROID_SYSTEM_VULKAN_ONLY)
  target_compile_definitions(vulkan PRIVATE LIBERTY_SYSTEM_VULKAN_ONLY=1)
else()
  target_link_libraries(vulkan PRIVATE adrenotools)
endif()
target_link_options(vulkan PRIVATE
  "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/exports.map"
  "-Wl,-Bsymbolic-functions"
  "-Wl,--exclude-libs,ALL")
set_target_properties(vulkan PROPERTIES LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/exports.map")

# Only these three runtime libraries are required for DRIVER_CUSTOM. The
# vendored build also defines optional BCn/file-redirection/GPU-mapping hooks.
install(TARGETS vulkan main_hook hook_impl c11_threads_compat LIBRARY DESTINATION lib/arm64-v8a)

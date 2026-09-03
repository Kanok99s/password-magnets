# =============================================================================
# PasswordMagnets
#   C++20 | static core library + CLI | CTest | install()/CPack
#   Cross-platform: Linux, macOS, Windows (Vcpkg / Conan / apt / brew)
# =============================================================================
cmake_minimum_required(VERSION 3.20)

project(PasswordMagnets
  VERSION 0.1.0
  DESCRIPTION "Password manager scaffold built on libsodium and nlohmann/json"
  LANGUAGES CXX
)

include(GNUInstallDirs)

# --- Global language settings ----------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Pick a sensible default build type for single-config generators.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the build type." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    Debug Release RelWithDebInfo MinSizeRel)
endif()

# --- Project options --------------------------------------------------------
option(PASSWORDMAGNETS_BUILD_TESTS "Build and register the CTest suite." ON)
option(PASSWORDMAGNETS_INSTALL "Generate install() and CPack packaging rules." ON)

# --- Dependencies ------------------------------------------------------------
# nlohmann/json ships a proper CMake package config on apt, Homebrew, Vcpkg and
# Conan, so one find_package call covers every ecosystem.
find_package(nlohmann_json CONFIG REQUIRED)

# libsodium is located by cmake/Libsodium.cmake: it tries the CMake config
# files installed by Vcpkg/Conan first, then falls back to pkg-config for
# distro packages (apt/brew) that only install libsodium.pc.
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Libsodium.cmake")
passwordmagnets_find_libsodium(PASSWORDMAGNETS_SODIUM_TARGET)

# --- Compiler hygiene ---------------------------------------------------------
if(MSVC)
  add_compile_options(/W4 /permissive- /utf-8)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# --- PasswordMagnets::core (static library) ----------------------------------
# Public headers live in include/, implementations live under src/.
add_library(passwordmagnets_core STATIC
  src/crypto/sodium.cpp
)
add_library(PasswordMagnets::core ALIAS passwordmagnets_core)

target_include_directories(passwordmagnets_core PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(passwordmagnets_core
  PUBLIC  ${PASSWORDMAGNETS_SODIUM_TARGET}
  PRIVATE nlohmann_json::nlohmann_json
)
set_target_properties(passwordmagnets_core PROPERTIES
  POSITION_INDEPENDENT_CODE ON
)

# --- passwordmagnets (executable) --------------------------------------------
add_executable(passwordmagnets src/main.cpp)
target_link_libraries(passwordmagnets PRIVATE PasswordMagnets::core)

# --- Tests --------------------------------------------------------------------
if(PASSWORDMAGNETS_BUILD_TESTS)
  include(CTest)
  if(BUILD_TESTING)
    add_subdirectory(tests)
  endif()
endif()

# --- Install & packaging --------------------------------------------------------
if(PASSWORDMAGNETS_INSTALL)
  install(TARGETS passwordmagnets_core
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  )
  install(TARGETS passwordmagnets
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
  )
  install(DIRECTORY include/passwordmagnets
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
  )

  set(CPACK_PACKAGE_NAME "PasswordMagnets")
  set(CPACK_PACKAGE_VENDOR "PasswordMagnets")
  set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
  set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
    "Password manager scaffold built on libsodium and nlohmann/json")
  set(CPACK_GENERATOR "ZIP;TGZ")
  include(CPack)
endif()

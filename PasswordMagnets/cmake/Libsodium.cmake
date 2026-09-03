# Locates libsodium and exports the imported target to link against.
#
# Different dependency ecosystems install libsodium differently, so we probe
# in a fixed order and stop at the first hit:
#
#   1. CMake package config files (find_package(libsodium CONFIG)):
#        - Vcpkg            -> target "libsodium::sodium"
#        - Conan CMakeDeps  -> target "libsodium::libsodium"
#        - upstream install -> target "libsodium::sodium" / "sodium"
#   2. pkg-config fallback (apt libsodium-dev, brew libsodium): those distro
#      packages ship libsodium.pc but NO CMake config, so this project
#      gracefully falls back to pkg_check_modules().
#
# Usage:
#   include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Libsodium.cmake")
#   passwordmagnets_find_libsodium(SODIUM_TARGET)
#   target_link_libraries(mytarget PRIVATE ${SODIUM_TARGET})

function(passwordmagnets_find_libsodium out_target)
  find_package(libsodium CONFIG QUIET)

  if(libsodium_FOUND)
    foreach(_candidate IN ITEMS libsodium::sodium libsodium::libsodium sodium)
      if(TARGET "${_candidate}")
        set(${out_target} "${_candidate}" PARENT_SCOPE)
        return()
      endif()
    endforeach()
  endif()

  # apt / brew / any environment where only libsodium.pc is installed.
  include(FindPkgConfig)
  pkg_check_modules(SODIUM_PC IMPORTED_TARGET libsodium)
  if(SODIUM_PC_FOUND)
    set(${out_target} PkgConfig::SODIUM_PC PARENT_SCOPE)
    return()
  endif()

  message(FATAL_ERROR
    "libsodium was not found.\n"
    "Install it with your preferred package manager and reconfigure:\n"
    "  Debian/Ubuntu : sudo apt install libsodium-dev\n"
    "  macOS/Homebrew: brew install libsodium pkgconf\n"
    "  Vcpkg         : vcpkg install libsodium\n"
    "  Conan         : conan install . --output-folder=build/conan --build=missing\n"
    "apt and Homebrew ship only libsodium.pc (no CMake config file); the\n"
    "pkg-config fallback is used automatically for those.")
endfunction()

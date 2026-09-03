# PasswordMagnets

Cross-platform **C++20** password manager built with **CMake** and a
**Qt6 Widgets** frontend. The vault layer links
[libsodium](https://doc.libsodium.org/) (Argon2id + XChaCha20-Poly1305) and
[nlohmann/json](https://github.com/nlohmann/json) through standard
`find_package()` calls, so the same build works with **Vcpkg**, **Conan**,
and system package managers (**apt** / **brew**) on Linux, macOS, and Windows.

## Layout

```
PasswordMagnets/
├── CMakeLists.txt              # targets, dependencies, install()/CPack
├── CMakePresets.json           # debug / release / windows-msvc presets
├── conanfile.txt               # Conan 2 recipe (CMakeDeps + CMakeToolchain)
├── vcpkg.json                  # Vcpkg manifest mode
├── cmake/
│   └── Libsodium.cmake         # find_package + pkg-config fallback for libsodium
├── include/
│   └── passwordmagnets/
│       └── crypto/sodium.hpp   # public headers (installed)
├── src/
│   ├── main.cpp                # entry point: QApplication + LoginDialog
│   ├── gui/                    # Qt6 Widgets UI (programmatic, no .ui files)
│   ├── crypto/                 # libsodium-backed crypto implementation
│   └── storage/                # VaultStore + HashTable (encrypted vault storage)
└── tests/
    ├── CMakeLists.txt
    ├── sodium_smoke.cpp        # CTest: verifies sodium_init() succeeds
    ├── hash_table_test.cpp     # CTest: container invariants
    └── vault_store_test.cpp    # CTest: CRUD + search ranking
```

## Requirements

- A C++20 compiler (GCC >= 10, Clang >= 12, MSVC >= 19.29 / VS 2022)
- CMake >= 3.20
- Qt 6 (Widgets module)
- libsodium and nlohmann/json (installed via any package manager below)

## Installing dependencies

### apt (Debian / Ubuntu)

```sh
sudo apt install build-essential cmake ninja-build pkg-config \
     qt6-base-dev libsodium-dev nlohmann-json3-dev
```

### Homebrew (macOS)

```sh
brew install cmake ninja pkgconf qt libsodium nlohmann-json
```

### Vcpkg (manifest mode picks up `vcpkg.json` automatically)

```sh
export VCPKG_ROOT=/path/to/vcpkg
cmake --preset debug \
      -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake
```

### Conan 2 (`conanfile.txt` included)

```sh
conan install . --output-folder=build/conan --build=missing
cmake --preset debug \
      -DCMAKE_TOOLCHAIN_FILE=build/conan/conan_toolchain.cmake
```

## Building, testing, running

```sh
cmake --preset debug
cmake --build --preset debug
ctest --preset debug            # sodium, hash table, vault store, checkpoint
./build/debug/passwordmagnets   # desktop UI: LoginDialog (create/unlock)
```

On Windows use `--preset windows-msvc` (Visual Studio 2022).

The executable has two modes:

- **`passwordmagnets`** - starts the Qt6 Widgets UI. On first launch no vault
  file exists, so the `LoginDialog` offers *Create Master Password* /
  *Create Vault*; afterwards it offers *Enter Master Password* / *Unlock*.
  The vault is stored at `QStandardPaths::AppDataLocation/vault.bin`.
  Because `QApplication::setQuitOnLastWindowClosed(false)` is in effect,
  closing the login dialog on success does not terminate the process - the
  app stays alive for the (upcoming) vault window transition.
- **`passwordmagnets --checkpoint`** - headless persistence self-check used by
  the `vault_checkpoint` CTest; it exits non-zero on any failure and requires
  no display or windowing system.

## Installing and packaging

```sh
cmake --install build/debug --prefix /desired/prefix
cpack --config build/debug/CPackConfig.cmake     # produces ZIP/TGZ archives
```

`install()` rules cover the `PasswordMagnets::core` static library, the
`passwordmagnets` executable, and the public headers under
`include/passwordmagnets/`.

## How libsodium is located

Different ecosystems install libsodium differently, so
`cmake/Libsodium.cmake` probes in order and stops at the first hit:

| Source                 | What is available                          | Resolved target     |
| ---------------------- | ------------------------------------------ | ------------------- |
| Vcpkg                  | CMake config (`libsodiumConfig.cmake`)     | `libsodium::sodium` |
| Conan (CMakeDeps)      | CMake config (`libsodium-config.cmake`)    | `libsodium::libsodium` |
| Upstream/other install | CMake config                               | `libsodium::sodium` / `sodium` |
| apt / brew             | only `libsodium.pc` (no CMake config)      | `PkgConfig::SODIUM_PC` |

`nlohmann_json` ships a CMake package config on every ecosystem, so a single
`find_package(nlohmann_json CONFIG REQUIRED)` suffices there.

## Adding code

- New **public API** goes in `include/passwordmagnets/...` and is installed.
- New **implementations** go in `src/crypto/`, `src/storage/`, or `src/gui/`
  and are added to the right target in `CMakeLists.txt` (crypto/storage ->
  `passwordmagnets_core`, GUI classes -> `passwordmagnets`, which links
  `Qt6::Widgets` and runs AUTOMOC for `Q_OBJECT` classes).

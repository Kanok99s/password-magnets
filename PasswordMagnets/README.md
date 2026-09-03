# PasswordMagnets

Cross-platform **C++20** password manager: a **Qt6 Widgets** desktop UI on top
of an encrypted vault backed by a from-scratch hash table, with cryptography
from **libsodium** and JSON from **nlohmann/json**. The project builds the same
way on Linux, macOS and Windows via **CMake** (system packages, **Vcpkg** or
**Conan 2**) and ships a **CTest** suite that runs headlessly — including in
the GitHub Actions workflow.

## Feature showcase

- **Encrypted, self-contained vault** — every entry lives in a single
  `vault.bin` file wrapped with authenticated encryption. There is no
  plaintext database anywhere on disk.
- **Create / unlock flow** — first launch offers *Create Master Password*;
  afterwards *Enter Master Password*. Wrong or empty passwords surface as
  inline errors, and a failed load never corrupts the in-memory vault.
- **Live search with deterministic ranking** — the vault window re-runs
  `VaultStore::search()` on every keystroke. Results are ordered by an
  explicit, deterministic model (exact match > earlier match offset > shorter
  field) built on a Knuth-Morris-Pratt substring scan.
- **Entry management** — table with Service / Username / masked Password /
  Actions columns, whole-row selection, and Add / Edit / Delete actions that
  persist to the encrypted file immediately.
- **Built-in password generator** — the add/edit dialog can mint random
  16–24 character passwords from `randombytes_buf` using rejection sampling
  (no modulo bias), with a Show/Hide reveal toggle.
- **Clipboard auto-clear** — copying a password arms a 20-second timer that
  scrubs the system clipboard if it still holds the secret. Only a SHA-256
  digest of the copied password is tracked — never a second plaintext copy.
- **Inactivity auto-lock** — the vault locks itself after 5 minutes without
  keyboard/mouse input, wiping plaintext rows, the in-memory vault copy and
  any watched clipboard secret before returning to the login dialog.
- **Encrypted backups** — a "Backup" menu on the vault window exports an
  encrypted snapshot of the current vault to any file (`Export Backup...`),
  and imports one back (`Import Backup...`) after decrypting it with its own
  master password, either merging it into the open vault (backup values win on
  duplicate service names) or replacing the vault wholesale.
- **Secret hygiene** — derived keys are non-copyable and wiped with
  `sodium_memzero()`; decrypted buffers are wiped on every exit path.
- **Headless persistence self-check** — `passwordmagnets --checkpoint` runs a
  full save/load/round-trip check with no windowing system, used by both CTest
  and CI.

## Architecture

The program is a small number of layers with narrow, testable interfaces:

```
 +------------------------ Qt6 Widgets UI (src/gui/) ------------------------+
 |  LoginDialog          MainWindow                 EntryDialog              |
 |  create / unlock      table + live search        add / edit entry         |
 |  masked password      copy (20 s auto-clear)     reveal toggle            |
 |                       lock / 5-min auto-lock     generate password        |
 +----------------------------------+----------------------------------------+
                                    |  CRUD + search (plaintext, in memory)
                                    v
 +--------------------------- VaultStore (src/storage/) ---------------------+
 |  entries_ : HashTable<std::string, Entry>   separate chaining, LF <= 0.7  |
 |  add / set / remove / get / contains / search (KMP) / allEntries         |
 |  serialize / deserialize via nlohmann::json                               |
 +----------------------------------+----------------------------------------+
                                    |  compact JSON document (UTF-8)
                                    v
 +---------------------------- CryptoEngine (src/crypto/) -------------------+
 |  deriveKey(master, salt)  ->  Argon2id  (crypto_pwhash, interactive)      |
 |  encrypt / decrypt        ->  authenticated secret-key encryption         |
 |  Key: non-copyable, sodium_memzero() on destruction / move-from           |
 +----------------------------------+----------------------------------------+
                                    |  16-byte salt | 24-byte nonce | MAC || ciphertext
                                    v
                              vault.bin   (one file, no header secrets)
```

### Memory flow and secret lifecycle

```
   master password  (typed in LoginDialog, held for the session)
           |
           |   deriveKey()  +  fresh random salt (16 B)
           v
   Key { 32 bytes }   ---- non-copyable; wiped with sodium_memzero() when
           |               destroyed or moved-from
           v
   encrypt:  JSON plaintext + fresh nonce (24 B)
             ->  salt | nonce | Poly1305 MAC || ciphertext
   decrypt:  MAC verified first; wrong key / tamper rejected silently

   Scrub points -----------------------------------------------
   · lock / 5-minute inactivity lock:
       clear displayed rows_, table cell data, the in-memory vault
       copy and the master password; scrub any watched clipboard secret
   · copy:  the clipboard is cleared again after 20 s; only a SHA-256
       digest of the password is kept while the timer is armed
   · decrypt:  the plaintext buffer is wiped on every exit path
```

## Why a custom hash table instead of `std::unordered_map`?

`VaultStore` keeps its entries in `HashTable` (`src/storage/HashTable.hpp`), a
header-only hash map written from scratch — no `std::unordered_map` is used
anywhere in the storage layer.

**The design.** The table uses *separate chaining* with power-of-two bucket
counts, starting at 8 buckets and doubling whenever an insert would push the
load factor above **0.7**:

```
   buckets_  (size always a power of two)
       0 -> [Node: ("github", Entry{...})] -> [Node: ...] -> nullptr
       1 -> nullptr
       2 -> [Node: ...] -> nullptr
      ...
   index_of(key) = hasher_(key) & (buckets_.size() - 1)   // mask, not modulo
```

Because the bucket count is a power of two, bucket selection is a bitwise AND
instead of a modulo; keys are compared with a supplied `KeyEqual`; nodes are
singly linked and inserted at the head of their bucket; iteration walks
buckets then chains, and copy / assignment use copy-and-swap.

**Why hand-roll it?**

1. **Deterministic, inspectable behaviour.** `std::unordered_map`'s bucket
   count and growth policy are implementation-defined, so the same code can
   behave differently across libc++, libstdc++ and the MSVC STL. Here the
   load-factor threshold, growth step and hash-to-index rule are explicit
   project policy, documented in one small header.
2. **A deliberate "data structures showcase".** The storage layer is the part
   of this codebase meant to show its craft: collision handling, dynamic
   resizing, custom iterators and deep-copy semantics are all implemented and
   unit-tested rather than assumed from a library.
3. **Nothing hidden behind the standard library.** Every memory allocation,
   rehash and equality check happens in code the project owns, which keeps
   behaviour predictable for a security-sensitive tool and makes the container
   trivially unit-testable at the byte level.
4. **The workload suits it.** A personal vault is small (dozens to hundreds of
   entries). The 0.7 load-factor bound keeps chains short, and the iteration
   order is never relied on for user-visible output — searches return an
   explicitly ranked, sorted vector and `allEntries()` sorts by service name —
   so any container-level nondeterminism is irrelevant by construction.

**Honest trade-offs.** `std::unordered_map` is heavily tuned by experts and
wins on raw throughput for large tables; this table allocates one heap node
per element (cache-unfriendly at scale), does not grow by arbitrary factors,
and has no heterogeneous lookup or hint-based insertion. Those gaps do not
matter for a vault, and if they ever did, the container is confined to
`VaultStore`'s private member — a deliberate, one-line swap, not an API change.

## Encryption design

The vault never stores raw credentials. Everything on disk is one
`[salt][nonce][MAC || ciphertext]` blob whose plaintext is the serialized JSON
of the entries.

### Key derivation — Argon2id

The master password is *never* used directly as a key. Instead, on every save
a fresh 16-byte random salt is generated and passed through
`crypto_pwhash` with the **Argon2id** variant (the Password Hashing
Competition winner) at libsodium's interactive cost limits:

```
key = Argon2id13(master_password, salt, OPSLIMIT_INTERACTIVE, MEMLIMIT_INTERACTIVE)
```

Why this matters:

- **Brute-force resistance.** A fast hash (SHA-2, MD5) can be tested billions
  of times per second on GPUs. Argon2id is *memory-hard*: an attacker must pay
  for memory as well as CPU, dramatically raising the cost of guessing weak
  master passwords.
- **Salt per file.** Because the salt is random and stored with the
  ciphertext, the same master password yields a different key for every vault
  file. Precomputed tables and cross-vault correlation are useless.
- **Interactive tuning.** The cost limits are chosen so unlock stays snappy on
  typical hardware while still slowing offline attacks. They can be raised for
  higher assurance.

### Authenticated encryption

The derived 32-byte key encrypts the JSON payload with libsodium's secret-key
box construction (XChaCha20-Poly1305): the stream cipher provides
confidentiality, and every ciphertext carries a Poly1305 message
authentication code.

- **Tamper detection.** The MAC is verified before any plaintext is trusted;
  a flipped byte, truncated file or wrong key fails authentication and the
  load is rejected without touching the in-memory vault.
- **Wide nonce space.** A fresh random 24-byte nonce per encryption makes
  nonce reuse practically impossible, so identical vaults still produce
  different ciphertexts.
- **One battle-tested library.** All primitives come from libsodium, a
  constant-time, audited implementation, rather than bespoke crypto.

### Secret handling hygiene

- Keys are derived transiently for each operation and kept in a **non-copyable,
  move-only `Key`** whose destructor and move operations wipe the bytes with
  `sodium_memzero()`.
- Decryption buffers are wrapped in an RAII guard so plaintext is wiped on
  *every* exit path, including error returns.
- Master passwords and displayed entry rows live only in memory for the
  duration of a session and are cleared when the vault locks.
- Copying a password is tracked by **digest only**, and the clipboard is
  cleared again after 20 seconds.

### Threat model

This protects the vault file **at rest** against offline attackers (stolen
file, lost laptop, backups) and gives integrity guarantees against tampering.
It does **not** defend a logged-in session against an attacker already running
code on the machine, shoulder-surfing, or keylogging — a strong, unique master
password is still the user's responsibility.

## Screenshots / demo

> Screenshots and an animated demo GIF belong here. Capture them and drop the
> files under `docs/screenshots/`, then reference them from this section, e.g.:
>
> ```
> ![Login dialog](docs/screenshots/login-dialog.png)
> ![Vault window with live search](docs/screenshots/vault-window.png)
> ![Add / edit entry with password generator](docs/screenshots/entry-dialog.png)
> ```

## Requirements

- A C++20 compiler: GCC ≥ 10, Clang ≥ 12, or MSVC ≥ 19.29 (Visual Studio 2022)
- CMake ≥ 3.20
- Qt 6 (Widgets module)
- libsodium and nlohmann/json (installed via any method below)

## Installing dependencies

### apt (Debian / Ubuntu) — also used by CI

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

## Building, testing and running

Presets cover the usual combinations (all defined in `CMakePresets.json`):

| Preset          | Generator                | Configuration        |
| --------------- | ------------------------ | -------------------- |
| `debug`         | Ninja                    | Debug                |
| `release`       | Ninja                    | Release              |
| `windows-msvc`  | Visual Studio 17 2022    | Debug (multi-config) |

```sh
cmake --preset debug          # configure
cmake --build --preset debug  # build
ctest --preset debug          # run the CTest suite
./build/debug/passwordmagnets # launch the desktop UI
```

The test suite (`ctest`) covers four checks, all headless:

- `sodium_smoke` — libsodium initializes.
- `hash_table_test` — container invariants (insert/find/remove/resize/iterators).
- `vault_store_test` — CRUD plus search ranking.
- `vault_checkpoint` — end-to-end persistence via `passwordmagnets --checkpoint`.

### Runtime modes

- **`passwordmagnets`** — the Qt6 Widgets UI. With no vault file present the
  login dialog offers *Create Master Password*; otherwise *Unlock*. The vault
  is stored at `QStandardPaths::AppDataLocation` (e.g. `%APPDATA%`, `~/Library`,
  `~/.local/share`) under `vault.bin`. After unlocking, the vault window offers
  live search, Add/Edit/Delete, per-row Copy (auto-clearing after 20 s) and
  Lock — manually, or automatically after 5 minutes of inactivity. The
  "Backup" menu exports the vault as an encrypted snapshot and imports a
  `.bin` backup back (merge or replace).
- **`passwordmagnets --checkpoint`** — the headless persistence self-check used
  by `vault_checkpoint`. Saves a seeded vault, reloads it, and verifies that a
  wrong password and a missing file fail gracefully without corrupting the
  loaded vault. Exits non-zero on any failure and needs no display.

## Continuous integration

`.github/workflows/build.yml` runs on every push and pull request on
`ubuntu-latest`: it installs `qt6-base-dev`, `libsodium-dev`,
`nlohmann-json3-dev`, `cmake` and `ninja-build`, configures the `release`
preset, builds, and runs the full CTest suite.

## Repository layout

```
PasswordMagnets/
├── .github/workflows/build.yml  # GitHub Actions CI (push + pull_request)
├── CMakeLists.txt               # targets, dependencies, install()/CPack
├── CMakePresets.json            # debug / release / windows-msvc presets
├── conanfile.txt                # Conan 2 recipe (CMakeDeps + CMakeToolchain)
├── vcpkg.json                   # Vcpkg manifest mode
├── cmake/
│   └── Libsodium.cmake          # find_package + pkg-config fallback
├── include/
│   └── passwordmagnets/
│       └── crypto/sodium.hpp    # public headers (installed)
├── src/
│   ├── main.cpp                 # entry point: QApplication + LoginDialog
│   ├── gui/                     # Qt6 Widgets UI (programmatic, no .ui):
│   │                            #   LoginDialog, MainWindow, EntryDialog
│   ├── crypto/                  # libsodium-backed crypto implementation
│   └── storage/                 # VaultStore + HashTable + KMP search
└── tests/
    ├── CMakeLists.txt
    ├── sodium_smoke.cpp
    ├── hash_table_test.cpp
    └── vault_store_test.cpp
```

## Adding code

- New **public API** goes in `include/passwordmagnets/...` and is installed.
- New **implementations** go in `src/crypto/`, `src/storage/` or `src/gui/` and
  are added to the matching target in `CMakeLists.txt` (crypto/storage →
  `passwordmagnets_core`; GUI classes → the `passwordmagnets` executable,
  which links `Qt6::Widgets` and runs AUTOMOC for `Q_OBJECT` classes).

## How libsodium is located

`cmake/Libsodium.cmake` probes ecosystems in order and stops at the first hit:

| Source                 | What is available                       | Resolved target         |
| ---------------------- | --------------------------------------- | ----------------------- |
| Vcpkg                  | CMake config (`libsodiumConfig.cmake`)  | `libsodium::sodium`     |
| Conan (CMakeDeps)      | CMake config (`libsodium-config.cmake`) | `libsodium::libsodium`  |
| Upstream / other       | CMake config                            | `libsodium::sodium` / `sodium` |
| apt / brew             | only `libsodium.pc`                     | `PkgConfig::SODIUM_PC`  |

`nlohmann_json` ships a CMake package config on every ecosystem, so a single
`find_package(nlohmann_json CONFIG REQUIRED)` suffices there.

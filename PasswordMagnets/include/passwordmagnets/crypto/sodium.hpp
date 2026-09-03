// PasswordMagnets - libsodium bootstrap.
//
// Public headers live under include/ so they are also the installed
// interface of the PasswordMagnets::core static library.

#pragma once

namespace passwordmagnets::crypto {

// Calls sodium_init() and reports whether libsodium is usable.
// Returns 0 on success (sodium_init() returned 0 or 1) or -1 on failure.
// Safe to call more than once: sodium_init() itself is idempotent.
[[nodiscard]] int init_sodium() noexcept;

}  // namespace passwordmagnets::crypto

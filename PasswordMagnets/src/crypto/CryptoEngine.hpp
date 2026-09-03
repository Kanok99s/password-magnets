// PasswordMagnets - symmetric encryption engine on top of libsodium.
//
// NOTE: this header lives next to its implementation (src/crypto/) for
// in-tree consumers of the PasswordMagnets::core library. Promote it to
// include/passwordmagnets/ if it becomes part of the installed API.

#pragma once

#include <sodium.h>

#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace passwordmagnets::crypto {

// Argon2id password-hashing salt (crypto_pwhash_SALTBYTES).
using Salt = std::array<unsigned char, crypto_pwhash_SALTBYTES>;

// XChaCha20-Poly1305 stream nonce (crypto_secretbox_NONCEBYTES).
using Nonce = std::array<unsigned char, crypto_secretbox_NONCEBYTES>;

// Self-contained encrypted record: salt, nonce, and ciphertext together.
struct EncryptedBlob {
  Salt salt;
  Nonce nonce;
  std::vector<unsigned char> ciphertext;  // Poly1305 MAC || ciphertext
};

// Move-only holder for secret key material. Its buffer is wiped with
// sodium_memzero() when the object goes out of scope, so secrets do not
// linger in memory after use.
class Key {
 public:
  static constexpr std::size_t kSize = crypto_secretbox_KEYBYTES;

  Key() noexcept : bytes_{} {}
  ~Key();

  Key(const Key&) = delete;
  Key& operator=(const Key&) = delete;
  Key(Key&&) noexcept;
  Key& operator=(Key&&) noexcept;

  [[nodiscard]] const unsigned char* data() const noexcept { return bytes_.data(); }
  [[nodiscard]] std::size_t size() const noexcept { return bytes_.size(); }

 private:
  friend class CryptoEngine;

  std::array<unsigned char, crypto_secretbox_KEYBYTES> bytes_;
};

class CryptoEngine {
 public:
  CryptoEngine();  // initializes libsodium; throws if that fails
  ~CryptoEngine() = default;

  CryptoEngine(const CryptoEngine&) = delete;
  CryptoEngine& operator=(const CryptoEngine&) = delete;

  // Fresh random salt / nonce from libsodium's secure RNG.
  [[nodiscard]] Salt generateSalt() const;
  [[nodiscard]] Nonce generateNonce() const;

  // Derives a 32-byte secretbox key from a master password using Argon2id
  // (crypto_pwhash) with libsodium's interactive limits.
  [[nodiscard]] Key deriveKey(std::string_view masterPassword, const Salt& salt) const;

  // Encrypts plaintext with XChaCha20-Poly1305 (crypto_secretbox_easy) and
  // returns an EncryptedBlob carrying its own random salt and nonce.
  [[nodiscard]] EncryptedBlob encrypt(std::string_view plaintext, const Key& key) const;

  // Decrypts a blob. Returns std::nullopt on a wrong key or tampered
  // ciphertext - authentication failures never throw.
  [[nodiscard]] std::optional<std::string> decrypt(const EncryptedBlob& blob, const Key& key) const;
};

}  // namespace passwordmagnets::crypto

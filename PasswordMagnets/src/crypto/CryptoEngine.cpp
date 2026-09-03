#include "CryptoEngine.hpp"

#include <stdexcept>

#include "passwordmagnets/crypto/sodium.hpp"

namespace passwordmagnets::crypto {
namespace {

// RAII guard that wipes a buffer with sodium_memzero() on scope exit.
class WipeGuard {
 public:
  WipeGuard(unsigned char* data, std::size_t size) noexcept : data_(data), size_(size) {}
  ~WipeGuard() {
    if (data_ != nullptr && size_ > 0) {
      sodium_memzero(data_, size_);
    }
  }
  WipeGuard(const WipeGuard&) = delete;
  WipeGuard& operator=(const WipeGuard&) = delete;

 private:
  unsigned char* data_;
  std::size_t size_;
};

}  // namespace

// --- Key ----------------------------------------------------------------------

Key::~Key() { sodium_memzero(bytes_.data(), bytes_.size()); }

Key::Key(Key&& other) noexcept : bytes_(other.bytes_) {
  // The moved-from object still holds the key material: wipe it so it can
  // never be observed after the move.
  sodium_memzero(other.bytes_.data(), other.bytes_.size());
}

Key& Key::operator=(Key&& other) noexcept {
  if (this != &other) {
    sodium_memzero(bytes_.data(), bytes_.size());
    bytes_ = other.bytes_;
    sodium_memzero(other.bytes_.data(), other.bytes_.size());
  }
  return *this;
}

// --- CryptoEngine ---------------------------------------------------------------

CryptoEngine::CryptoEngine() {
  // init_sodium() is idempotent (sodium_init() may be called repeatedly).
  // Fail loudly rather than running crypto without a seeded RNG.
  if (init_sodium() != 0) {
    throw std::runtime_error("CryptoEngine: libsodium could not be initialized");
  }
}

Salt CryptoEngine::generateSalt() const {
  Salt salt;
  randombytes_buf(salt.data(), salt.size());
  return salt;
}

Nonce CryptoEngine::generateNonce() const {
  Nonce nonce;
  randombytes_buf(nonce.data(), nonce.size());
  return nonce;
}

Key CryptoEngine::deriveKey(std::string_view masterPassword, const Salt& salt) const {
  Key key;

  // Never pass a (possibly null) data() pointer from an empty view: point at
  // a dummy byte instead when the password has zero length.
  const unsigned char empty_password = 0;
  const unsigned char* password = masterPassword.empty()
                                      ? &empty_password
                                      : reinterpret_cast<const unsigned char*>(
                                            masterPassword.data());
  const int rc = crypto_pwhash(
      key.bytes_.data(), key.bytes_.size(), password, masterPassword.size(),
      salt.data(), SODIUM_CRYPTO_PWHASH_OPSLIMIT_INTERACTIVE,
      SODIUM_CRYPTO_PWHASH_MEMLIMIT_INTERACTIVE, crypto_pwhash_ALG_ARGON2ID13);

  if (rc != 0) {
    // crypto_pwhash failed (e.g. out of memory): never hand out a partially
    // derived key, wipe whatever was written into the output buffer.
    sodium_memzero(key.bytes_.data(), key.bytes_.size());
    throw std::runtime_error(
        "CryptoEngine::deriveKey: crypto_pwhash failed (insufficient memory?)");
  }
  return key;
}

EncryptedBlob CryptoEngine::encrypt(std::string_view plaintext, const Key& key) const {
  EncryptedBlob blob;
  blob.salt = generateSalt();
  blob.nonce = generateNonce();

  // Point at a dummy byte for empty messages so libsodium never receives a
  // (possibly null) data() pointer from an empty string_view.
  const unsigned char empty_message = 0;
  const unsigned char* message = plaintext.empty()
                                     ? &empty_message
                                     : reinterpret_cast<const unsigned char*>(
                                           plaintext.data());

  blob.ciphertext.resize(plaintext.size() + crypto_secretbox_MACBYTES);
  if (crypto_secretbox_easy(blob.ciphertext.data(), message, plaintext.size(),
                            blob.nonce.data(), key.bytes_.data()) != 0) {
    sodium_memzero(blob.ciphertext.data(), blob.ciphertext.size());
    throw std::runtime_error("CryptoEngine::encrypt: crypto_secretbox_easy failed");
  }
  return blob;
}

std::optional<std::string> CryptoEngine::decrypt(const EncryptedBlob& blob,
                                                 const Key& key) const {
  // Even an empty message carries a MAC, so anything shorter is bogus.
  if (blob.ciphertext.size() < crypto_secretbox_MACBYTES) {
    return std::nullopt;
  }

  const std::size_t plaintext_size = blob.ciphertext.size() - crypto_secretbox_MACBYTES;
  std::vector<unsigned char> plaintext(plaintext_size);
  WipeGuard wipe(plaintext.data(), plaintext.size());  // wiped on every exit path

  if (crypto_secretbox_open_easy(plaintext.data(), blob.ciphertext.data(),
                                 blob.ciphertext.size(), blob.nonce.data(),
                                 key.bytes_.data()) != 0) {
    // Wrong key or tampered ciphertext: report failure, do not throw.
    return std::nullopt;
  }

  std::string result(reinterpret_cast<const char*>(plaintext.data()),
                     plaintext.size());
  return result;
}

}  // namespace passwordmagnets::crypto

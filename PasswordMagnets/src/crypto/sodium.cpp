#include "passwordmagnets/crypto/sodium.hpp"

#include <sodium.h>

namespace passwordmagnets::crypto {

int init_sodium() noexcept {
  // sodium_init() returns 0 on the first successful initialization, 1 when
  // libsodium was already initialized, and -1 if initialization failed.
  const int rc = sodium_init();
  return rc < 0 ? -1 : 0;
}

}  // namespace passwordmagnets::crypto

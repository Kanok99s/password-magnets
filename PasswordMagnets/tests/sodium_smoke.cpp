// CTest smoke test: verifies that the PasswordMagnets::core static library
// links against libsodium and that sodium_init() succeeds at runtime.

#include <cstdlib>
#include <iostream>

#include "passwordmagnets/crypto/sodium.hpp"

int main() {
  const int rc = passwordmagnets::crypto::init_sodium();
  if (rc != 0) {
    std::cerr << "sodium smoke test FAILED: init_sodium returned " << rc << '\n';
    return EXIT_FAILURE;
  }
  std::cout << "sodium smoke test passed (sodium_init == 0)\n";
  return EXIT_SUCCESS;
}

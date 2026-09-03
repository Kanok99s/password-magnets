// PasswordMagnets - application entry point.
//
// Prints a startup message and confirms libsodium initializes correctly:
// sodium_init() must succeed (the executable exits with code 0) for the
// application to start.

#include <cstdlib>
#include <iostream>

#include "passwordmagnets/crypto/sodium.hpp"

int main() {
  std::cout << "PasswordMagnets starting up...\n";

  std::cout << "Initializing libsodium... ";
  const int rc = passwordmagnets::crypto::init_sodium();
  if (rc != 0) {
    std::cerr << "FAILED (sodium_init returned " << rc << ")\n";
    return EXIT_FAILURE;
  }

  std::cout << "libsodium ready.\n";
  return EXIT_SUCCESS;
}

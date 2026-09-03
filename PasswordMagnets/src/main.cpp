// PasswordMagnets - application entry point.
//
// Besides a startup banner this executable runs the persistence checkpoint:
// it builds a vault, saves it to disk encrypted with a master password,
// reloads it with the correct password, then proves that a wrong password
// is rejected cleanly without corrupting the in-memory vault.

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <utility>

#include "passwordmagnets/crypto/sodium.hpp"
#include "storage/VaultStore.hpp"

namespace {

bool entry_matches(const passwordmagnets::storage::Entry& a,
                   const passwordmagnets::storage::Entry& b) {
  return a.service == b.service && a.username == b.username &&
         a.password == b.password && a.notes == b.notes;
}

// RAII cleanup so a failed checkpoint cannot leave the test file behind.
class ScopedFile {
 public:
  explicit ScopedFile(std::string path) : path_(std::move(path)) {}
  ~ScopedFile() { std::remove(path_.c_str()); }
  ScopedFile(const ScopedFile&) = delete;
  ScopedFile& operator=(const ScopedFile&) = delete;

 private:
  std::string path_;
};

bool persistence_check() {
  using passwordmagnets::storage::Entry;
  using passwordmagnets::storage::VaultStore;

  const std::string path = "vault_checkpoint.bin";
  const std::string master = "correct horse battery staple";
  ScopedFile cleanup(path);

  // 1. Seed a vault with three sample entries.
  Entry github;
  github.service = "github";
  github.username = "octocat";
  github.password = "s3cret";
  github.notes = "work";

  Entry dropbox;
  dropbox.service = "dropbox";
  dropbox.username = "mona";
  dropbox.password = "pa$$w0rd";
  dropbox.notes = "backups";

  Entry slack;
  slack.service = "slack";
  slack.username = "alice";
  slack.password = "dontuse123";
  slack.notes = "team chat";

  VaultStore vault;
  const bool seeded =
      vault.add(github) && vault.add(dropbox) && vault.add(slack) &&
      vault.size() == 3;
  if (!seeded) {
    std::cerr << "checkpoint: could not seed the vault\n";
    return false;
  }

  // 2. Encrypt to disk.
  const bool saved = vault.saveToFile(path, master);
  if (!saved) {
    std::cerr << "checkpoint: saveToFile failed\n";
    return false;
  }

  // 3. Round-trip with the correct master password.
  VaultStore loaded;
  const bool loaded_ok = loaded.loadFromFile(path, master);
  const auto verify_loaded = [&](const VaultStore& v) {
    const std::optional<Entry> gh = v.get("github");
    const std::optional<Entry> db = v.get("dropbox");
    const std::optional<Entry> sl = v.get("slack");
    return v.size() == 3 && gh.has_value() && db.has_value() && sl.has_value() &&
           entry_matches(*gh, github) && entry_matches(*db, dropbox) &&
           entry_matches(*sl, slack);
  };
  const bool roundtrip = loaded_ok && verify_loaded(loaded);
  if (!roundtrip) {
    std::cerr << "checkpoint: round-trip with the correct password failed\n";
    return false;
  }

  // 4. A wrong master password must be rejected without corrupting the
  //    vault that was loaded in step 3.
  const bool rejected = !loaded.loadFromFile(path, "definitely wrong");
  const bool intact = verify_loaded(loaded);
  if (!rejected || !intact) {
    std::cerr << "checkpoint: wrong password was not rejected gracefully\n";
    return false;
  }

  // 5. A missing file must also fail without throwing or mutating anything.
  const bool missing_ok = !loaded.loadFromFile("no_such_vault.bin", master) &&
                          verify_loaded(loaded);
  if (!missing_ok) {
    std::cerr << "checkpoint: missing-file load did not fail cleanly\n";
    return false;
  }

  std::cout << "  seeded 3 entries, encrypted vault -> '" << path << "'\n"
            << "  correct password: 3/3 entries restored intact\n"
            << "  wrong password:   rejected, vault unchanged\n"
            << "  missing file:     rejected cleanly\n";
  return true;
}

}  // namespace

int main() {
  std::cout << "PasswordMagnets starting up...\n";

  std::cout << "Initializing libsodium... ";
  const int rc = passwordmagnets::crypto::init_sodium();
  if (rc != 0) {
    std::cerr << "FAILED (sodium_init returned " << rc << ")\n";
    return EXIT_FAILURE;
  }
  std::cout << "libsodium ready.\n\n";

  std::cout << "Vault persistence checkpoint\n";
  std::cout << "-----------------------------\n";
  if (!persistence_check()) {
    std::cerr << "checkpoint FAILED\n";
    return EXIT_FAILURE;
  }
  std::cout << "checkpoint passed.\n";
  return EXIT_SUCCESS;
}

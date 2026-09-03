// PasswordMagnets - Qt6 Widgets application entry point.
//
//   passwordmagnets                -> desktop UI (sodium init + LoginDialog)
//   passwordmagnets --checkpoint   -> headless persistence self-check (CTest)
//
// The persistence checkpoint doubles as the CTest "vault_checkpoint": it
// builds a vault, saves it encrypted with a master password, reloads it with
// the correct password, then proves that a wrong password is rejected cleanly
// without corrupting the in-memory vault.

#include <QApplication>
#include <QCoreApplication>
#include <QDebug>
#include <QDialog>
#include <QMessageBox>

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <utility>

#include "gui/LoginDialog.hpp"
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

// Returns true when argv is exactly "--checkpoint".
bool wants_checkpoint(int argc, char** argv) {
  return argc == 2 && std::string(argv[1]) == "--checkpoint";
}

// Headless mode used by CTest: no QApplication, no windows.
int run_checkpoint() {
  std::cout << "PasswordMagnets persistence checkpoint\n"
            << "-------------------------------------\n";
  const int rc = passwordmagnets::crypto::init_sodium();
  if (rc != 0) {
    std::cerr << "sodium_init returned " << rc << '\n';
    return EXIT_FAILURE;
  }
  if (!persistence_check()) {
    std::cerr << "checkpoint FAILED\n";
    return EXIT_FAILURE;
  }
  std::cout << "checkpoint passed.\n";
  return EXIT_SUCCESS;
}

}  // namespace

int main(int argc, char** argv) {
  if (wants_checkpoint(argc, argv)) return run_checkpoint();

  QApplication app(argc, argv);
  QCoreApplication::setOrganizationName(QStringLiteral("PasswordMagnets"));
  QCoreApplication::setApplicationName(QStringLiteral("PasswordMagnets"));

  // Closing (or transitioning away from) a window must not terminate the app.
  // The login dialog closes itself on success and the next window takes over;
  // only an explicit quit() ends the process.
  QApplication::setQuitOnLastWindowClosed(false);

  const int rc = passwordmagnets::crypto::init_sodium();
  if (rc != 0) {
    QMessageBox::critical(
        nullptr, QStringLiteral("PasswordMagnets"),
        QStringLiteral("Could not initialize cryptography support "
                       "(sodium_init returned %1).")
            .arg(rc));
    return EXIT_FAILURE;
  }

  passwordmagnets::ui::LoginDialog dialog;
  dialog.show();

  // Successful create/unlock: the dialog closed itself (accept()), but the
  // process keeps running. The vault window will be constructed and shown
  // here in a later step.
  QObject::connect(&dialog, &passwordmagnets::ui::LoginDialog::authenticationSucceeded,
                   &dialog, [] {
                     qInfo("authenticated - handing off to the vault window");
                   });

  // The login window is the only one for now: closing it (window "X" or Esc)
  // finishes the dialog with Rejected and is an explicit "I want to quit".
  // A successful unlock finishes with Accepted, which must NOT quit the app
  // (that is the whole point of setQuitOnLastWindowClosed(false)).
  QObject::connect(&dialog, &QDialog::finished, &dialog,
                   [](int result) {
                     if (result != QDialog::Accepted) QCoreApplication::quit();
                   });

  return app.exec();
}

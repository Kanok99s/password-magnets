// PasswordMagnets - Qt6 Widgets application entry point.
//
//   passwordmagnets                -> desktop UI (sodium init + LoginDialog)
//   passwordmagnets --checkpoint   -> headless persistence self-check (CTest)
//
// Desktop lifecycle, driven by window transitions:
//   * LoginDialog runs first: no vault file -> "Create Master Password"
//     (creates and saves a fresh vault); vault present -> "Enter Master
//     Password" (loadFromFile with inline error on failure).
//   * On success the login dialog closes itself and the MainWindow (the
//     vault listing with live search and add/edit/delete) takes over.
//   * Lock Vault disposes of the vault window and returns to the login
//     dialog for the next unlock. Closing a window via "X"/Esc quits.
//   * QApplication::setQuitOnLastWindowClosed(false) is what makes those
//     window transitions possible without terminating the process.
//
// The persistence checkpoint doubles as the CTest "vault_checkpoint": it
// builds a vault, saves it encrypted with a master password, reloads it with
// the correct password, then proves that a wrong password is rejected cleanly
// without corrupting the in-memory vault.

#include <QApplication>
#include <QCoreApplication>
#include <QDialog>
#include <QMessageBox>
#include <QPointer>

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <utility>

#include "gui/LoginDialog.hpp"
#include "gui/MainWindow.hpp"
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

  // One login dialog reused across lock cycles; the vault window is created
  // fresh after every successful unlock.
  passwordmagnets::ui::LoginDialog login;
  QPointer<passwordmagnets::ui::MainWindow> vaultWindow;

  const auto openVaultWindow = [&] {
    // Finished with Accepted, so LoginDialog holds the unlocked vault and the
    // credentials that open it; hand copies to the vault window.
    auto* const win = new passwordmagnets::ui::MainWindow(
        login.vault(), login.masterPassword(), login.vaultPath());
    vaultWindow = win;

    // Lock: dispose of the vault window and return to the login dialog.
    QObject::connect(win, &passwordmagnets::ui::MainWindow::vaultLocked, win,
                     [&] {
                       if (vaultWindow) {
                         vaultWindow->hide();
                         vaultWindow->deleteLater();
                       }
                       login.prepareForLogin();
                       login.show();
                     });

    // Closing the vault window via the window manager exits the application.
    QObject::connect(win, &passwordmagnets::ui::MainWindow::windowClosed, win,
                     [] { QCoreApplication::quit(); });

    win->show();
  };

  // Login flow: Rejected ("X"/Esc) quits; Accepted (vault created/unlocked)
  // transitions to the vault window.
  QObject::connect(&login, &QDialog::finished, &login, [&](int result) {
    if (result == QDialog::Rejected) {
      QCoreApplication::quit();
      return;
    }
    openVaultWindow();
  });

  login.show();
  return app.exec();
}

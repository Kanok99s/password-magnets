// LoginDialog: the first screen of the PasswordMagnets desktop UI.
//
// The whole widget tree is assembled programmatically (no .ui files). On
// startup the dialog inspects the vault file on disk and switches between two
// states:
//
//   * Unlock - the vault file exists: "Enter Master Password" + an "Unlock"
//     button that calls VaultStore::loadFromFile().
//   * Create - no vault file yet: "Create Master Password" + a "Create Vault"
//     button that initializes a fresh VaultStore and saves it to disk.
//
// Wrong or empty passwords surface as inline red text under the input field.
// On success the dialog emits authenticationSucceeded() and closes itself
// with accept(); QApplication::setQuitOnLastWindowClosed(false) in main.cpp
// keeps the process alive so the caller can hand off to the next window.
#pragma once

#include <QDialog>
#include <QString>

#include "storage/VaultStore.hpp"

class QLabel;
class QLineEdit;
class QPushButton;
class QCloseEvent;

namespace passwordmagnets::ui {

class LoginDialog final : public QDialog {
  Q_OBJECT

 public:
  // When `vaultPath` is empty the default per-user vault location is used
  // (see defaultVaultPath()).
  explicit LoginDialog(QString vaultPath = QString(), QWidget* parent = nullptr);

  // Standard location for this machine's vault file, e.g. the AppData
  // directory on Windows/macOS/Linux plus "/vault.bin".
  static QString defaultVaultPath();

  // Re-arms this dialog for another authentication round after the vault was
  // locked: the previous session is discarded and the Create/Unlock state is
  // re-derived from the vault file currently on disk.
  void prepareForLogin();

  // --- Result state (valid once authenticationSucceeded() has fired) ------
  const storage::VaultStore& vault() const noexcept { return vault_; }
  QString vaultPath() const noexcept { return vaultPath_; }
  QString masterPassword() const noexcept { return masterPassword_; }
  bool authenticated() const noexcept { return authenticated_; }

 signals:
  // Emitted after the vault was created or unlocked successfully. The dialog
  // then closes itself (accept()); the process stays alive thanks to
  // QApplication::setQuitOnLastWindowClosed(false).
  void authenticationSucceeded();

 private:
  enum class Mode { kUnlock, kCreate };

  void buildUi();
  void configureFor(Mode mode);
  void submit();
  void showError(const QString& message);
  void clearError();

  Mode mode_ = Mode::kUnlock;
  bool authenticated_ = false;
  bool busy_ = false;  // guards against re-entrant submit() calls
  QString vaultPath_;
  QString masterPassword_;
  storage::VaultStore vault_;

  QLabel* promptLabel_ = nullptr;  // "Enter/Create Master Password"
  QLabel* hintLabel_ = nullptr;    // secondary guidance line
  QLabel* errorLabel_ = nullptr;   // inline red error message
  QLineEdit* passwordEdit_ = nullptr;
  QPushButton* actionButton_ = nullptr;  // "Unlock" / "Create Vault"

 protected:
  // Window "X" counts as cancel (see LoginDialog.cpp).
  void closeEvent(QCloseEvent* event) override;
};

}  // namespace passwordmagnets::ui

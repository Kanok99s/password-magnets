// MainWindow: the unlocked-vault screen of PasswordMagnets.
//
// A single programmatic widget tree (no .ui files):
//   * a live-search QLineEdit that re-runs VaultStore::search() (or lists
//     everything via allEntries() when empty) on every keystroke,
//   * a QTableWidget with Service | Username | Password | Actions columns,
//     whole-row selection and stretched columns,
//   * Add / Edit / Delete / Lock Vault actions, plus a per-row Copy button,
//   * a "Backup" menu bar with Export Backup... and Import Backup...
//     (encrypted snapshots via the vault's file format).
//
// The window owns a copy of the vault plus the credentials that unlock it, so
// every mutation can be written back to the encrypted file immediately
// (persist()). Locking emits vaultLocked(); closing via the window manager
// emits windowClosed() so the caller can decide how to proceed.
//
// Two self-protection features live here:
//   * Clipboard auto-clear: after a Copy the password is watched for 20 s and
//     scrubbed from the system clipboard if it is still there. Only a SHA-256
//     digest of the copied password is kept in memory, never a second copy of
//     the plaintext.
//   * Inactivity lock: after 5 minutes without keyboard/mouse input the
//     window locks itself via vaultLocked() so the caller can present the
//     login dialog again. Input is watched application-wide through an event
//     filter, installed on show and removed on hide so it is always gone
//     before the window is disposed of.
#pragma once

#include <QByteArray>
#include <QMainWindow>
#include <QPoint>
#include <QString>

#include <vector>

#include "storage/VaultStore.hpp"

class QCloseEvent;
class QEvent;
class QHideEvent;
class QLabel;
class QLineEdit;
class QPushButton;
class QShowEvent;
class QTableWidget;
class QTimer;

namespace passwordmagnets::ui {

class MainWindow final : public QMainWindow {
  Q_OBJECT

 public:
  MainWindow(storage::VaultStore vault, QString masterPassword,
             QString vaultPath, QWidget* parent = nullptr);

  const storage::VaultStore& vault() const noexcept { return vault_; }

 signals:
  // The user pressed "Lock Vault", or the inactivity timer fired. The caller
  // hides this window and returns to the login dialog.
  void vaultLocked();

  // The user closed this window via the window manager ("X" button).
  void windowClosed();

 protected:
  void closeEvent(QCloseEvent* event) override;
  void showEvent(QShowEvent* event) override;
  void hideEvent(QHideEvent* event) override;

  // Watches the whole application for keyboard/mouse input so inactivity can
  // be measured across the session (modal dialogs included). The filter is
  // installed on the QApplication in showEvent() and removed in hideEvent().
  bool eventFilter(QObject* watched, QEvent* event) override;

 private:
  void buildUi();
  void refreshTable();
  void updateActionButtons();
  int currentRow() const;
  void selectRowForService(const std::string& service);
  bool persist();  // write vault_ back to disk; warns the user on failure

  void onSearchChanged(const QString& text);
  void addEntry();
  void editEntry();
  void deleteEntry();
  void copyPassword(int row);

  // --- Backup (Export / Import) --------------------------------------------
  // "Export Backup...": write an encrypted snapshot of the current vault to a
  // file the user picks. Re-uses saveToFile(), which re-keys the copy with a
  // fresh salt + nonce under the session master password.
  void exportBackup();
  // "Import Backup...": decrypt a file the user picks using that file's own
  // master password into a throwaway VaultStore, then either merge the result
  // into the open vault (backup values win on duplicate service names) or
  // replace the vault contents. The merged vault is persisted immediately.
  void importBackup();

  // --- Security helpers ------------------------------------------------------
  void noteActivity();    // a keypress/mouse event: restart the idle timer
  void onIdleTimeout();   // the idle timer fired (5 min without input)
  void lockVault();       // scrub in-memory secrets, then emit vaultLocked()
  void clearCopiedPassword(bool notify);  // stop watch; clear clipboard if ours

  storage::VaultStore vault_;
  QString masterPassword_;
  QString vaultPath_;

  // Entries currently displayed, kept in row order.
  std::vector<storage::Entry> rows_;

  QLineEdit* searchEdit_ = nullptr;
  QLabel* countLabel_ = nullptr;
  QTableWidget* table_ = nullptr;
  QPushButton* addButton_ = nullptr;
  QPushButton* editButton_ = nullptr;
  QPushButton* deleteButton_ = nullptr;
  QPushButton* lockButton_ = nullptr;

  // Clipboard auto-clear.
  QTimer* clipboardTimer_ = nullptr;  // 20 s single-shot after Copy
  QByteArray copiedDigest_;           // SHA-256 of the password on watch

  // Inactivity auto-lock.
  QTimer* idleTimer_ = nullptr;  // 5 min single-shot, reset on input
  QPoint lastMousePos_;          // last seen global mouse position
};

}  // namespace passwordmagnets::ui

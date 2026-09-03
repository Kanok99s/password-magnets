// MainWindow: the unlocked-vault screen of PasswordMagnets.
//
// A single programmatic widget tree (no .ui files):
//   * a live-search QLineEdit that re-runs VaultStore::search() (or lists
//     everything via allEntries() when empty) on every keystroke,
//   * a QTableWidget with Service | Username | Password | Actions columns,
//     whole-row selection and stretched columns,
//   * Add / Edit / Delete / Lock Vault actions.
//
// The window owns a copy of the vault plus the credentials that unlock it, so
// every mutation can be written back to the encrypted file immediately
// (persist()). Locking emits vaultLocked(); closing via the window manager
// emits windowClosed() so the caller can decide how to proceed.
#pragma once

#include <QMainWindow>
#include <QString>

#include <vector>

#include "storage/VaultStore.hpp"

class QCloseEvent;
class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;

namespace passwordmagnets::ui {

class MainWindow final : public QMainWindow {
  Q_OBJECT

 public:
  MainWindow(storage::VaultStore vault, QString masterPassword,
             QString vaultPath, QWidget* parent = nullptr);

  const storage::VaultStore& vault() const noexcept { return vault_; }

 signals:
  // The user pressed "Lock Vault". The caller hides this window and returns
  // to the login dialog.
  void vaultLocked();

  // The user closed this window via the window manager ("X" button).
  void windowClosed();

 protected:
  void closeEvent(QCloseEvent* event) override;

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
};

}  // namespace passwordmagnets::ui

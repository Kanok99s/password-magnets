// EntryDialog: modal form for adding or editing a single credential.
//
// A small programmatic form (no .ui files) shared by MainWindow's "Add Entry"
// and "Edit Entry" actions:
//   * Service / Username / Password / Notes input fields.
//   * Password is masked by default with a Show/Hide toggle button.
//   * A "Generate Password" button fills the field with a random 16-24
//     character string drawn from libsodium's randombytes_buf() (OS-backed
//     cryptographically secure randomness).
//   * Accepting (Save) validates that a service name is present and the caller
//     reads the assembled storage::Entry via entry().
#pragma once

#include <QDialog>

#include "storage/VaultStore.hpp"

class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QToolButton;

namespace passwordmagnets::ui {

class EntryDialog final : public QDialog {
  Q_OBJECT

 public:
  explicit EntryDialog(QWidget* parent = nullptr);

  // Pre-fills the fields for editing an existing credential.
  void setEntry(const storage::Entry& entry);

  // The credential assembled from the current field contents.
  storage::Entry entry() const;

  // Validates the service field, then accepts (or shows an inline error).
  void accept() override;

 private:
  // Generates a random password and writes it into the password field.
  void generatePassword();

  QLineEdit* serviceEdit_ = nullptr;
  QLineEdit* usernameEdit_ = nullptr;
  QLineEdit* passwordEdit_ = nullptr;
  QToolButton* revealButton_ = nullptr;
  QPlainTextEdit* notesEdit_ = nullptr;
  QLabel* errorLabel_ = nullptr;
};

}  // namespace passwordmagnets::ui

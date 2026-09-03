// EntryDialog: modal add/edit form for a single credential.
//
// A small programmatic form (no .ui files) shared by MainWindow's "Add Entry"
// and "Edit Entry" actions. It validates that a service name is present and
// returns the entered values as a storage::Entry on accept().
#pragma once

#include <QDialog>

#include "storage/VaultStore.hpp"

class QLabel;
class QLineEdit;
class QPlainTextEdit;

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
  QLineEdit* serviceEdit_ = nullptr;
  QLineEdit* usernameEdit_ = nullptr;
  QLineEdit* passwordEdit_ = nullptr;
  QPlainTextEdit* notesEdit_ = nullptr;
  QLabel* errorLabel_ = nullptr;
};

}  // namespace passwordmagnets::ui

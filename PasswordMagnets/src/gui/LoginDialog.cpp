// LoginDialog implementation - programmatic Qt6 Widgets UI (no .ui files).
#include "gui/LoginDialog.hpp"

#include <QDir>
#include <QCloseEvent>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStandardPaths>
#include <QVBoxLayout>

#include <utility>

namespace passwordmagnets::ui {

namespace {

// Centralized colors so the accent and error states stay consistent.
constexpr const char* kAccent = "#4f46e5";         // indigo button
constexpr const char* kAccentHover = "#4338ca";
constexpr const char* kAccentPressed = "#3730a3";
constexpr const char* kHintColor = "#6b7280";      // secondary guidance text
constexpr const char* kErrorColor = "#c62828";     // dark red inline errors

QString actionButtonStyle() {
  return QStringLiteral(
      "QPushButton {"
      "  background-color: %1; color: #ffffff; border: none;"
      "  border-radius: 6px; padding: 10px 16px; font-weight: 600;"
      "}"
      "QPushButton:hover { background-color: %2; }"
      "QPushButton:pressed { background-color: %3; }"
      "QPushButton:disabled { background-color: #b7bcef; color: #eef0ff; }")
      .arg(QLatin1String(kAccent), QLatin1String(kAccentHover),
           QLatin1String(kAccentPressed));
}

}  // namespace

QString LoginDialog::defaultVaultPath() {
  const QString dataDir =
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  if (dataDir.isEmpty()) return QDir::current().filePath(QStringLiteral("vault.bin"));
  return dataDir + QStringLiteral("/vault.bin");
}

LoginDialog::LoginDialog(QString vaultPath, QWidget* parent) : QDialog(parent) {
  vaultPath_ = vaultPath.isEmpty() ? defaultVaultPath() : vaultPath;
  setWindowTitle(QStringLiteral("PasswordMagnets"));

  // Make sure the vault's directory exists before we probe for the file.
  QDir().mkpath(QFileInfo(vaultPath_).absolutePath());

  buildUi();

  // The vault file decides which state the dialog starts in.
  const Mode mode = QFile::exists(vaultPath_) ? Mode::kUnlock : Mode::kCreate;
  configureFor(mode);
}

void LoginDialog::prepareForLogin() {
  // Drop the previous session: the file on disk is the source of truth and
  // decides whether the next round creates or unlocks a vault.
  vault_ = storage::VaultStore();
  masterPassword_.clear();
  configureFor(QFile::exists(vaultPath_) ? Mode::kUnlock : Mode::kCreate);
}

void LoginDialog::buildUi() {
  auto* const layout = new QVBoxLayout(this);
  layout->setContentsMargins(28, 24, 28, 20);
  layout->setSpacing(10);

  // Primary prompt: "Enter Master Password" / "Create Master Password".
  promptLabel_ = new QLabel(this);
  promptLabel_->setObjectName(QStringLiteral("promptLabel"));
  promptLabel_->setWordWrap(true);
  QFont promptFont = font();
  promptFont.setPointSizeF(13.0);
  promptFont.setBold(true);
  promptLabel_->setFont(promptFont);

  // Secondary guidance line (gray, wraps).
  hintLabel_ = new QLabel(this);
  hintLabel_->setObjectName(QStringLiteral("hintLabel"));
  hintLabel_->setWordWrap(true);
  hintLabel_->setStyleSheet(
      QStringLiteral("color: %1;").arg(QLatin1String(kHintColor)));

  // Master password input, always masked.
  passwordEdit_ = new QLineEdit(this);
  passwordEdit_->setObjectName(QStringLiteral("passwordEdit"));
  passwordEdit_->setEchoMode(QLineEdit::Password);
  passwordEdit_->setClearButtonEnabled(true);

  // Inline error label: hidden until a submission fails.
  errorLabel_ = new QLabel(this);
  errorLabel_->setObjectName(QStringLiteral("errorLabel"));
  errorLabel_->setWordWrap(true);
  errorLabel_->setStyleSheet(
      QStringLiteral("color: %1;").arg(QLatin1String(kErrorColor)));
  errorLabel_->hide();

  // Primary action: "Unlock" / "Create Vault".
  actionButton_ = new QPushButton(this);
  actionButton_->setObjectName(QStringLiteral("actionButton"));
  actionButton_->setCursor(Qt::PointingHandCursor);
  actionButton_->setStyleSheet(actionButtonStyle());

  layout->addWidget(promptLabel_);
  layout->addWidget(hintLabel_);
  layout->addWidget(passwordEdit_);
  layout->addWidget(errorLabel_);
  layout->addSpacing(2);
  layout->addWidget(actionButton_);

  // Submit from the button click or from pressing Enter in the field.
  connect(actionButton_, &QPushButton::clicked, this, &LoginDialog::submit);
  connect(passwordEdit_, &QLineEdit::returnPressed, this, &LoginDialog::submit);

  setMinimumWidth(360);
}

void LoginDialog::configureFor(Mode mode) {
  mode_ = mode;
  authenticated_ = false;
  busy_ = false;
  clearError();
  passwordEdit_->clear();
  passwordEdit_->setFocus();
  actionButton_->setEnabled(true);

  if (mode == Mode::kUnlock) {
    setWindowTitle(QStringLiteral("Unlock Vault"));
    promptLabel_->setText(QStringLiteral("Enter Master Password"));
    hintLabel_->setText(QStringLiteral(
        "Your vault is stored encrypted on this computer. Enter the master "
        "password that was used to create it."));
    passwordEdit_->setPlaceholderText(QStringLiteral("Master password"));
    actionButton_->setText(QStringLiteral("Unlock"));
  } else {
    setWindowTitle(QStringLiteral("Create Vault"));
    promptLabel_->setText(QStringLiteral("Create Master Password"));
    hintLabel_->setText(QStringLiteral(
        "No vault exists yet, so this dialog will create one and encrypt it "
        "with the master password you choose. It cannot be recovered if you "
        "forget it."));
    passwordEdit_->setPlaceholderText(QStringLiteral("Choose a master password"));
    actionButton_->setText(QStringLiteral("Create Vault"));
  }
}

void LoginDialog::submit() {
  if (busy_ || authenticated_) return;
  clearError();

  const QString password = passwordEdit_->text();
  if (password.isEmpty()) {
    showError(mode_ == Mode::kCreate
                  ? QStringLiteral("A master password is required to create a vault.")
                  : QStringLiteral("Enter your master password to unlock the vault."));
    passwordEdit_->setFocus();
    return;
  }

  busy_ = true;
  actionButton_->setEnabled(false);

  storage::VaultStore vault;
  bool ok = false;
  if (mode_ == Mode::kCreate) {
    // Fresh (empty) vault written to disk under the new master password.
    ok = vault.saveToFile(vaultPath_.toStdString(), password.toStdString());
    if (!ok) {
      showError(QStringLiteral(
          "Could not create the vault file. Check the destination path and "
          "its permissions."));
    }
  } else {
    // Existing vault: decryption failure (wrong password / damaged file) is
    // reported inline and leaves the in-memory vault untouched.
    ok = vault.loadFromFile(vaultPath_.toStdString(), password.toStdString());
    if (!ok) {
      showError(QStringLiteral(
          "Incorrect master password, or the vault file is damaged."));
    }
  }

  if (!ok) {
    busy_ = false;
    actionButton_->setEnabled(true);
    passwordEdit_->selectAll();
    passwordEdit_->setFocus();
    return;
  }

  vault_ = std::move(vault);
  masterPassword_ = password;
  authenticated_ = true;
  busy_ = false;

  emit authenticationSucceeded();
  accept();  // hides the dialog; the app stays alive for the next window
}

void LoginDialog::showError(const QString& message) {
  errorLabel_->setText(message);
  errorLabel_->show();
}

void LoginDialog::clearError() {
  errorLabel_->clear();
  errorLabel_->hide();
}

void LoginDialog::closeEvent(QCloseEvent* event) {
  // A window-manager close (the "X" button) counts as cancel, exactly like
  // Esc, so main()'s finished() handler quits the app. Successful unlocks
  // leave through accept(), which hides the dialog without a close event.
  reject();
  event->accept();
}

}  // namespace passwordmagnets::ui

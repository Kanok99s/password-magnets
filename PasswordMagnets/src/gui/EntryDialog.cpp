// EntryDialog implementation - programmatic Qt6 form layout (no .ui files).
#include "gui/EntryDialog.hpp"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include <sodium.h>

namespace passwordmagnets::ui {

namespace {
constexpr const char* kErrorColor = "#c62828";
constexpr const char* kAccent = "#4f46e5";
constexpr const char* kAccentHover = "#4338ca";
constexpr const char* kAccentPressed = "#3730a3";

QString primaryButtonStyle() {
  return QStringLiteral(
      "QPushButton { background-color: %1; color: #ffffff; border: none;"
      "  border-radius: 6px; padding: 8px 18px; font-weight: 600; }"
      "QPushButton:hover { background-color: %2; }"
      "QPushButton:pressed { background-color: %3; }")
      .arg(QLatin1String(kAccent), QLatin1String(kAccentHover),
           QLatin1String(kAccentPressed));
}

QString secondaryButtonStyle() {
  return QStringLiteral(
      "QPushButton { background-color: #ffffff; color: %1;"
      "  border: 1px solid %1; border-radius: 6px; padding: 6px 14px; }"
      "QPushButton:hover { background-color: #eef2ff; }"
      "QPushButton:pressed { background-color: #e0e7ff; }")
      .arg(QLatin1String(kAccent));
}

QString revealButtonStyle() {
  return QStringLiteral(
      "QToolButton { background: transparent; border: none; color: %1;"
      "  font-weight: 600; padding: 4px 8px; }"
      "QToolButton:hover { color: %2; }")
      .arg(QLatin1String(kAccent), QLatin1String(kAccentHover));
}

// Draws one uniformly distributed index in [0, count) using rejection
// sampling, so modulo bias cannot skew the generated passwords.
int randomIndex(int count) {
  const int threshold = 256 - (256 % count);
  unsigned char byte = 0;
  do {
    randombytes_buf(&byte, sizeof(byte));
  } while (byte >= threshold);
  return static_cast<int>(byte) % count;
}
}  // namespace

EntryDialog::EntryDialog(QWidget* parent) : QDialog(parent) {
  setWindowTitle(QStringLiteral("Add Entry"));
  setMinimumWidth(420);

  auto* const root = new QVBoxLayout(this);
  root->setContentsMargins(20, 18, 20, 16);
  root->setSpacing(10);

  auto* const form = new QFormLayout;
  form->setSpacing(8);
  form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

  serviceEdit_ = new QLineEdit(this);
  serviceEdit_->setObjectName(QStringLiteral("serviceEdit"));
  serviceEdit_->setPlaceholderText(QStringLiteral("e.g. github"));
  form->addRow(QStringLiteral("Service"), serviceEdit_);

  usernameEdit_ = new QLineEdit(this);
  usernameEdit_->setObjectName(QStringLiteral("usernameEdit"));
  usernameEdit_->setPlaceholderText(QStringLiteral("e.g. octocat"));
  form->addRow(QStringLiteral("Username"), usernameEdit_);

  // --- Password row: masked field + Show/Hide toggle + generator. ----------
  passwordEdit_ = new QLineEdit(this);
  passwordEdit_->setObjectName(QStringLiteral("passwordEdit"));
  passwordEdit_->setEchoMode(QLineEdit::Password);
  passwordEdit_->setClearButtonEnabled(true);

  revealButton_ = new QToolButton(this);
  revealButton_->setObjectName(QStringLiteral("revealButton"));
  revealButton_->setCheckable(true);
  revealButton_->setCursor(Qt::PointingHandCursor);
  revealButton_->setToolTip(QStringLiteral("Show or hide the password"));
  revealButton_->setText(QStringLiteral("Show"));
  revealButton_->setStyleSheet(revealButtonStyle());

  auto* const passwordRow = new QHBoxLayout;
  passwordRow->setSpacing(4);
  passwordRow->addWidget(passwordEdit_, /*stretch=*/1);
  passwordRow->addWidget(revealButton_);
  form->addRow(QStringLiteral("Password"), passwordRow);

  connect(revealButton_, &QToolButton::toggled, this, [this](bool shown) {
    passwordEdit_->setEchoMode(shown ? QLineEdit::Normal : QLineEdit::Password);
    revealButton_->setText(shown ? QStringLiteral("Hide")
                                 : QStringLiteral("Show"));
  });

  // --- Password generator row. ---------------------------------------------
  auto* const generateButton =
      new QPushButton(QStringLiteral("Generate Password"), this);
  generateButton->setObjectName(QStringLiteral("generateButton"));
  generateButton->setCursor(Qt::PointingHandCursor);
  generateButton->setToolTip(
      QStringLiteral("Fill with a random 16-24 character password from the "
                     "system cryptographic random source (libsodium)."));
  generateButton->setStyleSheet(secondaryButtonStyle());
  connect(generateButton, &QPushButton::clicked, this,
          &EntryDialog::generatePassword);

  auto* const generateRow = new QHBoxLayout;
  generateRow->addWidget(generateButton);
  generateRow->addStretch(1);
  form->addRow(QString(), generateRow);

  // --- Notes. ---------------------------------------------------------------
  notesEdit_ = new QPlainTextEdit(this);
  notesEdit_->setObjectName(QStringLiteral("notesEdit"));
  notesEdit_->setPlaceholderText(QStringLiteral("Optional notes..."));
  notesEdit_->setFixedHeight(64);
  form->addRow(QStringLiteral("Notes"), notesEdit_);

  errorLabel_ = new QLabel(this);
  errorLabel_->setObjectName(QStringLiteral("errorLabel"));
  errorLabel_->setWordWrap(true);
  errorLabel_->setStyleSheet(
      QStringLiteral("color: %1;").arg(QLatin1String(kErrorColor)));
  errorLabel_->hide();

  auto* const buttons =
      new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                           this);
  buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("Save"));
  buttons->button(QDialogButtonBox::Ok)->setStyleSheet(primaryButtonStyle());
  buttons->button(QDialogButtonBox::Cancel)->setAutoDefault(false);

  root->addLayout(form);
  root->addWidget(errorLabel_);
  root->addWidget(buttons);

  connect(buttons, &QDialogButtonBox::accepted, this, &EntryDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &EntryDialog::reject);

  serviceEdit_->setFocus();
}

void EntryDialog::setEntry(const storage::Entry& entry) {
  serviceEdit_->setText(QString::fromStdString(entry.service));
  usernameEdit_->setText(QString::fromStdString(entry.username));
  passwordEdit_->setText(QString::fromStdString(entry.password));
  notesEdit_->setPlainText(QString::fromStdString(entry.notes));
}

storage::Entry EntryDialog::entry() const {
  storage::Entry e;
  e.service = serviceEdit_->text().trimmed().toStdString();
  e.username = usernameEdit_->text().toStdString();
  e.password = passwordEdit_->text().toStdString();
  e.notes = notesEdit_->toPlainText().toStdString();
  return e;
}

void EntryDialog::generatePassword() {
  // Printable alphabet: upper + lower case, digits, and common symbols.
  // Deliberately omits ambiguous quotes/backslash/space so the result is
  // easy to retype on foreign keyboards.
  static constexpr char kAlphabet[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz"
      "0123456789"
      "!@#$%^&*()-_=+[]{}<>?";
  constexpr int kAlphabetSize = static_cast<int>(sizeof(kAlphabet) - 1);

  // Length is itself random, uniform over 16..24 (nine values).
  constexpr int kMinLength = 16;
  constexpr int kLengthRange = 9;  // 16, 17, ..., 24
  const int length = kMinLength + randomIndex(kLengthRange);

  QString password;
  password.reserve(length);
  for (int i = 0; i < length; ++i) {
    password.append(QLatin1Char(kAlphabet[randomIndex(kAlphabetSize)]));
  }
  passwordEdit_->setText(password);

  // Briefly reveal the fresh password so the user can read it back, then
  // restore whatever visibility state they had chosen.
  const bool stayVisible = revealButton_->isChecked();
  passwordEdit_->setEchoMode(QLineEdit::Normal);
  revealButton_->setChecked(true);
  if (!stayVisible) {
    QTimer::singleShot(2000, this, [this] {
      passwordEdit_->setEchoMode(QLineEdit::Password);
      revealButton_->setChecked(false);
    });
  }
}

void EntryDialog::accept() {
  if (serviceEdit_->text().trimmed().isEmpty()) {
    errorLabel_->setText(QStringLiteral("A service name is required."));
    errorLabel_->show();
    serviceEdit_->setFocus();
    return;
  }
  QDialog::accept();
}

}  // namespace passwordmagnets::ui

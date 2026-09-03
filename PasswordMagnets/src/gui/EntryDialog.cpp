// EntryDialog implementation - programmatic Qt6 form layout (no .ui files).
#include "gui/EntryDialog.hpp"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace passwordmagnets::ui {

namespace {
constexpr const char* kErrorColor = "#c62828";
constexpr const char* kAccent = "#4f46e5";
constexpr const char* kAccentHover = "#4338ca";
constexpr const char* kAccentPressed = "#3730a3";

QString buttonStyle() {
  return QStringLiteral(
      "QPushButton { background-color: %1; color: #ffffff; border: none;"
      "  border-radius: 6px; padding: 8px 18px; font-weight: 600; }"
      "QPushButton:hover { background-color: %2; }"
      "QPushButton:pressed { background-color: %3; }")
      .arg(QLatin1String(kAccent), QLatin1String(kAccentHover),
           QLatin1String(kAccentPressed));
}
}  // namespace

EntryDialog::EntryDialog(QWidget* parent) : QDialog(parent) {
  setWindowTitle(QStringLiteral("Add Entry"));
  setMinimumWidth(380);

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

  passwordEdit_ = new QLineEdit(this);
  passwordEdit_->setObjectName(QStringLiteral("passwordEdit"));
  passwordEdit_->setEchoMode(QLineEdit::Password);
  form->addRow(QStringLiteral("Password"), passwordEdit_);

  auto* const revealBox = new QCheckBox(QStringLiteral("Show password"), this);
  form->addRow(QString(), revealBox);

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
      new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("Save"));
  buttons->button(QDialogButtonBox::Ok)->setStyleSheet(buttonStyle());
  buttons->button(QDialogButtonBox::Cancel)->setAutoDefault(false);

  root->addLayout(form);
  root->addWidget(errorLabel_);
  root->addWidget(buttons);

  connect(revealBox, &QCheckBox::toggled, this, [this](bool shown) {
    passwordEdit_->setEchoMode(shown ? QLineEdit::Normal : QLineEdit::Password);
  });
  connect(buttons, &QDialogButtonBox::accepted, this, &EntryDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &EntryDialog::reject);
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

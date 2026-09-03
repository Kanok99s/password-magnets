// MainWindow implementation - programmatic Qt6 Widgets UI (no .ui files).
#include "gui/MainWindow.hpp"

#include <QAbstractItemView>
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QStringList>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include <algorithm>
#include <utility>

#include "gui/EntryDialog.hpp"

namespace passwordmagnets::ui {

namespace {

// Columns of the entry table (Service | Username | Password | Actions).
enum Column : int { kService = 0, kUsername = 1, kPassword = 2, kActions = 3 };

constexpr const char* kAccent = "#4f46e5";
constexpr const char* kAccentHover = "#4338ca";
constexpr const char* kAccentPressed = "#3730a3";
constexpr const char* kDanger = "#dc2626";

QString primaryButtonStyle() {
  return QStringLiteral(
      "QPushButton { background-color: %1; color: #ffffff; border: none;"
      "  border-radius: 6px; padding: 8px 16px; font-weight: 600; }"
      "QPushButton:hover { background-color: %2; }"
      "QPushButton:pressed { background-color: %3; }"
      "QPushButton:disabled { background-color: #b7bcef; color: #eef0ff; }")
      .arg(QLatin1String(kAccent), QLatin1String(kAccentHover),
           QLatin1String(kAccentPressed));
}

QString secondaryButtonStyle() {
  return QStringLiteral(
      "QPushButton { background-color: #ffffff; color: #1f2937;"
      "  border: 1px solid #d1d5db; border-radius: 6px; padding: 8px 14px; }"
      "QPushButton:hover { background-color: #f3f4f6; }"
      "QPushButton:pressed { background-color: #e5e7eb; }"
      "QPushButton:disabled { color: #9ca3af; border-color: #e5e7eb; }");
}

QString dangerButtonStyle() {
  return QStringLiteral(
      "QPushButton { background-color: #ffffff; color: %1;"
      "  border: 1px solid %1; border-radius: 6px; padding: 8px 14px; }"
      "QPushButton:hover { background-color: #fef2f2; }"
      "QPushButton:pressed { background-color: #fee2e2; }"
      "QPushButton:disabled { color: #fca5a5; border-color: #fecaca; }")
      .arg(QLatin1String(kDanger));
}

// Non-editable, selectable cell text (so users can click a row without
// accidentally mutating a value).
QTableWidgetItem* cellItem(const QString& text) {
  auto* item = new QTableWidgetItem(text);
  item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  return item;
}

// Bullets for the masked password column; the count mirrors the real length
// (capped) so an empty password reads as an empty cell.
QString maskedPassword(const QString& password) {
  const int visible =
      std::min(password.size(), static_cast<qsizetype>(16));
  return QString(visible, QChar(0x2022));
}

}  // namespace

MainWindow::MainWindow(storage::VaultStore vault, QString masterPassword,
                       QString vaultPath, QWidget* parent)
    : QMainWindow(parent),
      vault_(std::move(vault)),
      masterPassword_(std::move(masterPassword)),
      vaultPath_(std::move(vaultPath)) {
  setWindowTitle(QStringLiteral("PasswordMagnets - Vault"));
  setMinimumSize(680, 440);
  resize(880, 580);

  buildUi();
  refreshTable();
}

void MainWindow::buildUi() {
  auto* const central = new QWidget(this);
  setCentralWidget(central);

  auto* const root = new QVBoxLayout(central);
  root->setContentsMargins(16, 14, 16, 12);
  root->setSpacing(10);

  // --- Search row: live filter on the left, entry count on the right. -----
  auto* const searchRow = new QHBoxLayout;
  searchRow->setSpacing(10);

  searchEdit_ = new QLineEdit(central);
  searchEdit_->setObjectName(QStringLiteral("searchEdit"));
  searchEdit_->setPlaceholderText(QStringLiteral("Search service or username..."));
  searchEdit_->setClearButtonEnabled(true);

  countLabel_ = new QLabel(central);
  countLabel_->setObjectName(QStringLiteral("countLabel"));
  countLabel_->setStyleSheet(QStringLiteral("color: #6b7280;"));
  countLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

  searchRow->addWidget(searchEdit_, /*stretch=*/1);
  searchRow->addWidget(countLabel_);

  // --- Entry table. ---------------------------------------------------------
  table_ = new QTableWidget(0, Column::kActions + 1, central);
  table_->setObjectName(QStringLiteral("entryTable"));
  table_->setHorizontalHeaderLabels(QStringList{
      QStringLiteral("Service"), QStringLiteral("Username"),
      QStringLiteral("Password"), QStringLiteral("Actions")});
  table_->verticalHeader()->setVisible(false);
  table_->verticalHeader()->setDefaultSectionSize(38);
  table_->setSelectionBehavior(QAbstractItemView::SelectRows);
  table_->setSelectionMode(QAbstractItemView::SingleSelection);
  table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  table_->setAlternatingRowColors(true);
  table_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

  // --- Action row: Add/Edit/Delete on the left, Lock Vault on the right. ---
  auto* const actionRow = new QHBoxLayout;
  actionRow->setSpacing(8);

  addButton_ = new QPushButton(QStringLiteral("Add Entry"), central);
  addButton_->setObjectName(QStringLiteral("addButton"));
  addButton_->setCursor(Qt::PointingHandCursor);
  addButton_->setStyleSheet(primaryButtonStyle());

  editButton_ = new QPushButton(QStringLiteral("Edit Entry"), central);
  editButton_->setObjectName(QStringLiteral("editButton"));
  editButton_->setCursor(Qt::PointingHandCursor);
  editButton_->setStyleSheet(secondaryButtonStyle());

  deleteButton_ = new QPushButton(QStringLiteral("Delete Entry"), central);
  deleteButton_->setObjectName(QStringLiteral("deleteButton"));
  deleteButton_->setCursor(Qt::PointingHandCursor);
  deleteButton_->setStyleSheet(dangerButtonStyle());

  lockButton_ = new QPushButton(QStringLiteral("Lock Vault"), central);
  lockButton_->setObjectName(QStringLiteral("lockButton"));
  lockButton_->setCursor(Qt::PointingHandCursor);
  lockButton_->setStyleSheet(secondaryButtonStyle());

  actionRow->addWidget(addButton_);
  actionRow->addWidget(editButton_);
  actionRow->addWidget(deleteButton_);
  actionRow->addStretch(1);
  actionRow->addWidget(lockButton_);

  root->addLayout(searchRow);
  root->addWidget(table_, /*stretch=*/1);
  root->addLayout(actionRow);

  // --- Wiring. ---------------------------------------------------------------
  connect(searchEdit_, &QLineEdit::textChanged, this,
          &MainWindow::onSearchChanged);
  connect(addButton_, &QPushButton::clicked, this, &MainWindow::addEntry);
  connect(editButton_, &QPushButton::clicked, this, &MainWindow::editEntry);
  connect(deleteButton_, &QPushButton::clicked, this, &MainWindow::deleteEntry);
  connect(lockButton_, &QPushButton::clicked, this,
          [this] { emit vaultLocked(); });
  connect(table_->selectionModel(), &QItemSelectionModel::selectionChanged,
          this, [this] { updateActionButtons(); });
}

void MainWindow::onSearchChanged(const QString& /*text*/) {
  refreshTable();
}

void MainWindow::refreshTable() {
  const QString query = searchEdit_->text().trimmed();
  rows_ = query.isEmpty() ? vault_.allEntries()
                          : vault_.search(query.toStdString());

  const int rowCount = static_cast<int>(rows_.size());

  // Reset the row count first so widgets from previous rows are released
  // before cells are repopulated.
  table_->setRowCount(0);
  table_->setRowCount(rowCount);

  for (int row = 0; row < rowCount; ++row) {
    const storage::Entry& e = rows_[static_cast<std::size_t>(row)];
    table_->setItem(row, Column::kService,
                    cellItem(QString::fromStdString(e.service)));
    table_->setItem(row, Column::kUsername,
                    cellItem(QString::fromStdString(e.username)));
    table_->setItem(row, Column::kPassword,
                    cellItem(maskedPassword(QString::fromStdString(e.password))));

    auto* const copyButton = new QPushButton(QStringLiteral("Copy"), table_);
    copyButton->setCursor(Qt::PointingHandCursor);
    copyButton->setStyleSheet(
        QStringLiteral(
            "QPushButton { border: none; background: transparent; color: %1;"
            "  font-weight: 600; padding: 4px 10px; }"
            "QPushButton:hover { text-decoration: underline; }")
            .arg(QLatin1String(kAccent)));
    const int rowIndex = row;  // stable copy for the lambda below
    connect(copyButton, &QPushButton::clicked, this,
            [this, rowIndex] { copyPassword(rowIndex); });
    table_->setCellWidget(row, Column::kActions, copyButton);
  }

  // Live feedback in the header line.
  const int total = static_cast<int>(vault_.size());
  if (query.isEmpty()) {
    countLabel_->setText(
        rowCount == 1 ? QStringLiteral("%1 entry").arg(rowCount)
                      : QStringLiteral("%1 entries").arg(rowCount));
  } else {
    countLabel_->setText(
        QStringLiteral("%1 of %2 entries").arg(rowCount).arg(total));
  }

  updateActionButtons();
}

void MainWindow::updateActionButtons() {
  const bool hasSelection = currentRow() >= 0;
  editButton_->setEnabled(hasSelection);
  deleteButton_->setEnabled(hasSelection);
}

int MainWindow::currentRow() const {
  const int row = table_->currentRow();
  if (row < 0 || row >= static_cast<int>(rows_.size())) return -1;
  return row;
}

void MainWindow::selectRowForService(const std::string& service) {
  for (int row = 0; row < static_cast<int>(rows_.size()); ++row) {
    if (rows_[static_cast<std::size_t>(row)].service == service) {
      table_->selectRow(row);
      table_->scrollToItem(table_->item(row, Column::kService));
      return;
    }
  }
}

bool MainWindow::persist() {
  const bool ok = vault_.saveToFile(vaultPath_.toStdString(),
                                    masterPassword_.toStdString());
  if (!ok) {
    QMessageBox::critical(
        this, QStringLiteral("Save Failed"),
        QStringLiteral("The vault could not be written back to disk. Your "
                       "change is only in memory for now."));
  }
  return ok;
}

void MainWindow::addEntry() {
  EntryDialog dialog(this);
  dialog.setWindowTitle(QStringLiteral("Add Entry"));
  if (dialog.exec() != QDialog::Accepted) return;

  const storage::Entry entry = dialog.entry();
  if (vault_.contains(entry.service)) {
    QMessageBox::warning(
        this, QStringLiteral("Duplicate Service"),
        QStringLiteral("An entry for \"%1\" already exists.")
            .arg(QString::fromStdString(entry.service)));
    return;
  }
  if (!vault_.add(entry)) {
    QMessageBox::warning(this, QStringLiteral("Add Failed"),
                         QStringLiteral("The entry could not be added."));
    return;
  }
  persist();

  refreshTable();
  selectRowForService(entry.service);
  statusBar()->showMessage(QStringLiteral("Added \"%1\".")
                               .arg(QString::fromStdString(entry.service)),
                           3000);
}

void MainWindow::editEntry() {
  const int row = currentRow();
  if (row < 0) {
    statusBar()->showMessage(QStringLiteral("Select an entry to edit."), 3000);
    return;
  }

  const storage::Entry original = rows_[static_cast<std::size_t>(row)];

  EntryDialog dialog(this);
  dialog.setWindowTitle(QStringLiteral("Edit Entry"));
  dialog.setEntry(original);
  if (dialog.exec() != QDialog::Accepted) return;

  const storage::Entry next = dialog.entry();
  if (next.service != original.service && vault_.contains(next.service)) {
    QMessageBox::warning(
        this, QStringLiteral("Duplicate Service"),
        QStringLiteral("An entry for \"%1\" already exists.")
            .arg(QString::fromStdString(next.service)));
    return;
  }
  if (!vault_.set(next)) {
    QMessageBox::warning(this, QStringLiteral("Edit Failed"),
                         QStringLiteral("The entry could not be updated."));
    return;
  }
  persist();

  refreshTable();
  selectRowForService(next.service);
  statusBar()->showMessage(QStringLiteral("Updated \"%1\".")
                               .arg(QString::fromStdString(next.service)),
                           3000);
}

void MainWindow::deleteEntry() {
  const int row = currentRow();
  if (row < 0) {
    statusBar()->showMessage(QStringLiteral("Select an entry to delete."),
                             3000);
    return;
  }

  const storage::Entry entry = rows_[static_cast<std::size_t>(row)];
  const auto choice = QMessageBox::question(
      this, QStringLiteral("Delete Entry"),
      QStringLiteral("Delete the entry for \"%1\"?")
          .arg(QString::fromStdString(entry.service)),
      QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
  if (choice != QMessageBox::Yes) return;

  if (!vault_.remove(entry.service)) {
    QMessageBox::warning(this, QStringLiteral("Delete Failed"),
                         QStringLiteral("The entry could not be removed."));
    return;
  }
  persist();

  refreshTable();
  statusBar()->showMessage(QStringLiteral("Deleted \"%1\".")
                               .arg(QString::fromStdString(entry.service)),
                           3000);
}

void MainWindow::copyPassword(int row) {
  if (row < 0 || row >= static_cast<int>(rows_.size())) return;
  const storage::Entry& e = rows_[static_cast<std::size_t>(row)];
  QApplication::clipboard()->setText(QString::fromStdString(e.password));
  statusBar()->showMessage(
      QStringLiteral("Password for \"%1\" copied to the clipboard.")
          .arg(QString::fromStdString(e.service)),
      3000);
}

void MainWindow::closeEvent(QCloseEvent* event) {
  // A window-manager close means "exit the application"; main.cpp handles the
  // rest. (Locking, by contrast, goes through vaultLocked() and does not fire
  // a close event.)
  emit windowClosed();
  event->accept();
}

}  // namespace passwordmagnets::ui

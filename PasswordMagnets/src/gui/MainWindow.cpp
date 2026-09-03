// MainWindow implementation - programmatic Qt6 Widgets UI (no .ui files).
#include "gui/MainWindow.hpp"

#include <QAbstractButton>
#include <QAbstractItemView>
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QHideEvent>
#include <QInputDialog>
#include <QItemSelectionModel>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPushButton>
#include <QShowEvent>
#include <QStringList>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
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

// A copied password is scrubbed from the clipboard 20 s later, provided it is
// still there (see clearCopiedPassword()).
constexpr int kClipboardClearMs = 20 * 1000;

// The vault locks itself after 5 minutes without keyboard/mouse input.
constexpr int kIdleLockMs = 5 * 60 * 1000;

// Non-reversible fingerprint used to recognise "the password we copied" on the
// clipboard without keeping a second plaintext copy in memory.
QByteArray sha256(const QByteArray& data) {
  return QCryptographicHash::hash(data, QCryptographicHash::Sha256);
}

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

  // --- Clipboard auto-clear timer (20 s single-shot, armed by Copy). -------
  clipboardTimer_ = new QTimer(this);
  clipboardTimer_->setSingleShot(true);
  connect(clipboardTimer_, &QTimer::timeout, this,
          [this] { clearCopiedPassword(/*notify=*/true); });

  // --- Inactivity auto-lock timer (5 min single-shot, reset on input). -----
  idleTimer_ = new QTimer(this);
  idleTimer_->setSingleShot(true);
  connect(idleTimer_, &QTimer::timeout, this, &MainWindow::onIdleTimeout);

  // The application-wide input watch (eventFilter) is installed in
  // showEvent() and removed in hideEvent(), so it is always gone before this
  // window is deleted.
}

void MainWindow::buildUi() {
  // --- Menu bar: encrypted backup export / import. -------------------------
  auto* const backupMenu = menuBar()->addMenu(QStringLiteral("Backup"));
  QAction* const exportAction =
      backupMenu->addAction(QStringLiteral("Export Backup..."), this,
                            &MainWindow::exportBackup,
                            QKeySequence(QStringLiteral("Ctrl+E")));
  exportAction->setStatusTip(QStringLiteral(
      "Write an encrypted snapshot of the current vault to a file"));
  QAction* const importAction =
      backupMenu->addAction(QStringLiteral("Import Backup..."), this,
                            &MainWindow::importBackup,
                            QKeySequence(QStringLiteral("Ctrl+I")));
  importAction->setStatusTip(QStringLiteral(
      "Load an encrypted backup and merge or replace the current vault"));
  statusBar();  // create early so the status tips above have somewhere to land

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
  lockButton_->setToolTip(QStringLiteral(
      "Lock now (the vault also locks itself after 5 minutes of inactivity)"));
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
  connect(lockButton_, &QPushButton::clicked, this, &MainWindow::lockVault);
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
    copyButton->setToolTip(
        QStringLiteral("Copy the password to the clipboard (auto-clears after "
                       "20 seconds)"));
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

void MainWindow::exportBackup() {
  // Suggest a timestamped filename so a typical "make a snapshot now" click
  // needs no typing; the dialog still lets the user pick any path/filename.
  const QString suggested = QStringLiteral("passwordmagnets-backup-%1.bin")
                                .arg(QDateTime::currentDateTime().toString(
                                    QStringLiteral("yyyyMMdd-HHmmss")));
  const QString path = QFileDialog::getSaveFileName(
      this, QStringLiteral("Export Backup"), QDir::home().filePath(suggested),
      QStringLiteral("Encrypted vault backup (*.bin)"));
  if (path.isEmpty()) return;  // cancelled

  // Re-uses saveToFile(): the snapshot is a normal encrypted vault file with
  // the same layout, freshly keyed with a random salt + nonce under the
  // session master password.
  if (vault_.saveToFile(path.toStdString(), masterPassword_.toStdString())) {
    statusBar()->showMessage(
        QStringLiteral("Backup exported to %1.")
            .arg(QDir::toNativeSeparators(path)),
        5000);
    return;
  }
  QMessageBox::critical(this, QStringLiteral("Export Failed"),
                        QStringLiteral("The backup could not be written. The "
                                       "destination may not be writable."));
}

void MainWindow::importBackup() {
  const QString path = QFileDialog::getOpenFileName(
      this, QStringLiteral("Import Backup"), QDir::homePath(),
      QStringLiteral("Encrypted vault backup (*.bin);;All files (*)"));
  if (path.isEmpty()) return;  // cancelled

  bool accepted = false;
  QString password = QInputDialog::getText(
      this, QStringLiteral("Import Backup"),
      QStringLiteral("Enter the master password that unlocks this backup:"),
      QLineEdit::Password, QString(), &accepted);
  if (!accepted) return;  // cancelled
  if (password.isEmpty()) {
    password.clear();
    QMessageBox::warning(
        this, QStringLiteral("Import Backup"),
        QStringLiteral("A master password is required to decrypt the backup."));
    return;
  }

  // Decrypt into a throwaway store first, so a wrong password or a corrupt
  // file can never clobber the open vault. Only once the whole file validates
  // do we touch the live store below.
  storage::VaultStore backup;
  const bool loaded =
      backup.loadFromFile(path.toStdString(), password.toStdString());
  password.clear();  // drop the transient copy as soon as it has been used
  if (!loaded) {
    QMessageBox::critical(
        this, QStringLiteral("Import Failed"),
        QStringLiteral("The backup could not be decrypted. The master "
                       "password may be wrong, or the file is not a valid "
                       "vault backup."));
    return;
  }

  const int count = static_cast<int>(backup.size());
  QMessageBox choice(this);
  choice.setIcon(QMessageBox::Question);
  choice.setWindowTitle(QStringLiteral("Import Backup"));
  choice.setText(QStringLiteral("The backup contains %1 %2. How should it be "
                                "applied to the open vault?")
                     .arg(count)
                     .arg(count == 1 ? QStringLiteral("entry")
                                     : QStringLiteral("entries")));
  choice.setInformativeText(
      QStringLiteral("Merge adds the backup's entries, and entries that "
                     "already exist are overwritten by the backup.\n"
                     "Replace discards the current entries first.\n"
                     "Either way the vault is saved to disk immediately."));
  auto* const mergeButton =
      choice.addButton(QStringLiteral("Merge"), QMessageBox::AcceptRole);
  auto* const replaceButton =
      choice.addButton(QStringLiteral("Replace"), QMessageBox::AcceptRole);
  auto* const cancelButton =
      choice.addButton(QStringLiteral("Cancel"), QMessageBox::RejectRole);
  choice.setDefaultButton(mergeButton);  // non-destructive default
  choice.exec();

  const QAbstractButton* const clicked = choice.clickedButton();
  if (clicked == nullptr || clicked == cancelButton) return;  // cancelled

  if (clicked == replaceButton) {
    // backup already holds the fully validated contents of the file, so move
    // it wholesale: no re-serialization and no per-entry copies.
    vault_ = std::move(backup);
    persist();
    refreshTable();
    statusBar()->showMessage(
        QStringLiteral("Vault replaced with backup (%1 %2).")
            .arg(count)
            .arg(count == 1 ? QStringLiteral("entry")
                            : QStringLiteral("entries")),
        5000);
    return;
  }

  // Merge: walk the backup in deterministic (alphabetical) order. New service
  // names are added; on a collision the backup's value wins (set() upserts).
  int added = 0;
  int overwritten = 0;
  for (const storage::Entry& e : backup.allEntries()) {
    if (vault_.contains(e.service)) {
      ++overwritten;
    } else {
      ++added;
    }
    vault_.set(e);
  }
  persist();
  refreshTable();
  statusBar()->showMessage(
      QStringLiteral("Merged backup into vault (%1 added, %2 overwritten).")
          .arg(added)
          .arg(overwritten),
      5000);
}

void MainWindow::copyPassword(int row) {
  if (row < 0 || row >= static_cast<int>(rows_.size())) return;
  const storage::Entry& e = rows_[static_cast<std::size_t>(row)];
  const QString password = QString::fromStdString(e.password);

  auto* const clipboard = QApplication::clipboard();
  clipboard->setText(password);

  // Track only a digest of what we copied, never a second plaintext copy, so
  // the 20 s sweep (clearCopiedPassword) can tell whether the clipboard still
  // holds it.
  copiedDigest_ = sha256(password.toUtf8());
  clipboardTimer_->start(kClipboardClearMs);

  statusBar()->showMessage(
      QStringLiteral("Password for \"%1\" copied; clipboard clears in 20 s.")
          .arg(QString::fromStdString(e.service)),
      3000);
}

void MainWindow::clearCopiedPassword(bool notify) {
  clipboardTimer_->stop();
  if (copiedDigest_.isEmpty()) return;  // nothing of ours on watch

  // Only clear when the clipboard still holds exactly what we copied: if the
  // user pasted something over it in the meantime, that text is theirs.
  const bool stillOurs =
      sha256(QApplication::clipboard()->text().toUtf8()) == copiedDigest_;
  copiedDigest_.clear();
  if (!stillOurs) return;

  QApplication::clipboard()->clear();
  if (notify) {
    statusBar()->showMessage(
        QStringLiteral("Copied password removed from the clipboard."), 3000);
  }
}

void MainWindow::noteActivity() {
  if (!isVisible()) return;  // hidden/locking: nothing to keep alive
  idleTimer_->start(kIdleLockMs);
}

void MainWindow::onIdleTimeout() {
  // A modal child (add/edit entry, delete confirmation) parks the user's
  // focus mid-operation; defer the lock rather than locking underneath it.
  if (QApplication::activeModalWidget() != nullptr) {
    idleTimer_->start(kIdleLockMs);
    return;
  }
  lockVault();
}

void MainWindow::lockVault() {
  // 1. If the clipboard still holds a password we copied, scrub it now so a
  //    lock never strands a plaintext secret on the system clipboard.
  clearCopiedPassword(/*notify=*/false);
  idleTimer_->stop();

  // 2. Drop the sensitive in-memory copies the window keeps: the plaintext
  //    rows_ vector, the widget cells mirroring it, and this vault/master
  //    password copy. The window is disposed right after vaultLocked(), but
  //    scrubbing now means no secrets linger while deletion is pending.
  rows_.clear();
  rows_.shrink_to_fit();
  table_->clearContents();
  table_->setRowCount(0);
  vault_.clear();
  masterPassword_.clear();

  // 3. Hand back to the caller (main.cpp hides the window, disposes of it,
  //    and shows the login dialog again).
  emit vaultLocked();
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event) {
  switch (event->type()) {
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::Wheel:
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
      noteActivity();
      break;
    case QEvent::MouseMove: {
      // Only moves to a *new* position count: widgets can emit a stream of
      // move events, and a mouse parked over the window must not keep the
      // vault unlocked forever.
      const auto* const move = static_cast<const QMouseEvent*>(event);
      const QPoint pos = move->globalPosition().toPoint();
      if (pos != lastMousePos_) {
        lastMousePos_ = pos;
        noteActivity();
      }
      break;
    }
    default:
      break;
  }
  return QMainWindow::eventFilter(watched, event);
}

void MainWindow::showEvent(QShowEvent* event) {
  QMainWindow::showEvent(event);
  // Watch every window and child for keyboard/mouse input so the idle timer
  // reflects real user activity (modal dialogs included). Idempotent: Qt
  // ignores a duplicate install of the same filter.
  QApplication::instance()->installEventFilter(this);
  idleTimer_->start(kIdleLockMs);
}

void MainWindow::hideEvent(QHideEvent* event) {
  QMainWindow::hideEvent(event);
  // Stop tracking input before we vanish (this always precedes deletion, so
  // the filter can never outlive the window it points back to).
  if (auto* const app = QApplication::instance(); app != nullptr) {
    app->removeEventFilter(this);
  }
  idleTimer_->stop();
}

void MainWindow::closeEvent(QCloseEvent* event) {
  // A window-manager close means "exit the application"; main.cpp handles the
  // rest. (Locking, by contrast, goes through vaultLocked() and does not fire
  // a close event.) Leaving must not strand a copied password on the system
  // clipboard, so scrub it first.
  clearCopiedPassword(/*notify=*/false);
  emit windowClosed();
  event->accept();
}

}  // namespace passwordmagnets::ui

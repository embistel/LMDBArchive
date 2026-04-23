#include "ui/MainWindow.h"
#include "app/AppController.h"
#include "ui/PreviewPane.h"
#include "ui/ExtractDialog.h"
#include "ui/ProgressDialog.h"
#include "ui/PropertiesDialog.h"
#include "ui/StatsDialog.h"
#include "model/ArchiveTreeNode.h"
#include "core/PathUtil.h"

#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QSplitter>
#include <QVBoxLayout>
#include <QTreeView>
#include <QTableView>
#include <QLineEdit>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QHeaderView>
#include <QMenu>
#include <QApplication>
#include <QClipboard>

MainWindow::MainWindow(AppController* controller, QWidget* parent)
    : QMainWindow(parent), controller_(controller) {
    setWindowTitle(QStringLiteral("LMDB Archive"));
    resize(1100, 700);

    folderModel_ = new FolderTreeModel(this);
    entryModel_ = new EntryTableModel(this);

    setupMenus();
    setupToolbar();
    setupCentralWidget();
    setupStatusBar();
    connectSignals();

    updateTitle();
}

MainWindow::~MainWindow() = default;

void MainWindow::setupMenus() {
    // File menu
    auto* fileMenu = menuBar()->addMenu(tr("&File"));

    actOpen_ = fileMenu->addAction(tr("&Open Archive..."));
    actCreate_ = fileMenu->addAction(tr("&Create Archive from Folder..."));
    fileMenu->addSeparator();
    actExtract_ = fileMenu->addAction(tr("&Extract..."));
    fileMenu->addSeparator();
    actClose_ = fileMenu->addAction(tr("&Close Archive"));
    fileMenu->addSeparator();
    fileMenu->addAction(tr("E&xit"), this, &QWidget::close);

    // Edit menu
    auto* editMenu = menuBar()->addMenu(tr("&Edit"));
    actDelete_ = editMenu->addAction(tr("&Delete"));
    editMenu->addSeparator();
    editMenu->addAction(tr("Select &All"), this, [this]() {
        entryTable_->selectAll();
    });
    actRefresh_ = editMenu->addAction(tr("&Refresh"));

    // View menu
    auto* viewMenu = menuBar()->addMenu(tr("&View"));
    viewMenu->addAction(tr("&Details"), this, [this]() {
        // Already in details mode with QTableView
    });
    viewMenu->addSeparator();
    viewMenu->addAction(toolbar_->toggleViewAction());

    // Tools menu
    auto* toolsMenu = menuBar()->addMenu(tr("&Tools"));
    actVerify_ = toolsMenu->addAction(tr("&Verify Archive"));
    actStats_ = toolsMenu->addAction(tr("&Statistics..."));

    // Help menu
    auto* helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(tr("&About"), this, [this]() {
        QMessageBox::about(this, tr("About LMDB Archive"),
            tr("LMDB Archive v1.0.0\n\n"
               "LMDB-based archive explorer and manager.\n"
               "7-Zip style interface for LMDB archives."));
    });
}

void MainWindow::setupToolbar() {
    toolbar_ = addToolBar(tr("Main Toolbar"));
    toolbar_->setMovable(false);

    actOpen_ = toolbar_->addAction(tr("Open"));
    actCreate_ = toolbar_->addAction(tr("Create"));
    actExtract_ = toolbar_->addAction(tr("Extract"));
    toolbar_->addSeparator();
    actAddFile_ = toolbar_->addAction(tr("Add File"));
    actAddFolder_ = toolbar_->addAction(tr("Add Folder"));
    actDelete_ = toolbar_->addAction(tr("Delete"));
    toolbar_->addSeparator();
    actUp_ = toolbar_->addAction(tr("Up"));
    actRefresh_ = toolbar_->addAction(tr("Refresh"));
    actVerify_ = toolbar_->addAction(tr("Verify"));
}

void MainWindow::setupCentralWidget() {
    auto* centralSplitter = new QSplitter(Qt::Vertical, this);

    // Top: address bar + tree/list
    auto* topWidget = new QWidget(this);
    auto* topLayout = new QVBoxLayout(topWidget);
    topLayout->setContentsMargins(0, 0, 0, 0);

    // Address bar
    addressEdit_ = new QLineEdit(this);
    addressEdit_->setPlaceholderText(tr("Archive path..."));
    topLayout->addWidget(addressEdit_);

    // Horizontal splitter: folder tree | file list
    mainSplitter_ = new QSplitter(Qt::Horizontal, this);

    folderTree_ = new QTreeView(this);
    folderTree_->setModel(folderModel_);
    folderTree_->setHeaderHidden(true);
    folderTree_->setMinimumWidth(180);
    folderTree_->setMaximumWidth(300);
    mainSplitter_->addWidget(folderTree_);

    entryTable_ = new QTableView(this);
    entryTable_->setModel(entryModel_);
    entryTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    entryTable_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    entryTable_->setSortingEnabled(true);
    entryTable_->horizontalHeader()->setStretchLastSection(true);
    entryTable_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    entryTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    entryTable_->setContextMenuPolicy(Qt::CustomContextMenu);
    mainSplitter_->addWidget(entryTable_);

    mainSplitter_->setStretchFactor(0, 1);
    mainSplitter_->setStretchFactor(1, 3);

    topLayout->addWidget(mainSplitter_);

    centralSplitter->addWidget(topWidget);

    // Bottom: preview pane
    previewPane_ = new PreviewPane(this);
    previewPane_->setMaximumHeight(250);
    centralSplitter->addWidget(previewPane_);

    centralSplitter->setStretchFactor(0, 3);
    centralSplitter->setStretchFactor(1, 1);

    setCentralWidget(centralSplitter);
}

void MainWindow::setupStatusBar() {
    statusLabel_ = new QLabel(tr("No archive open"), this);
    statusBar()->addWidget(statusLabel_, 1);
}

void MainWindow::connectSignals() {
    // File actions
    connect(actOpen_, &QAction::triggered, this, &MainWindow::openArchive);
    connect(actCreate_, &QAction::triggered, this, &MainWindow::createArchive);
    connect(actExtract_, &QAction::triggered, this, &MainWindow::extractArchive);
    connect(actClose_, &QAction::triggered, this, &MainWindow::closeArchive);

    // Edit actions
    connect(actDelete_, &QAction::triggered, this, &MainWindow::deleteSelected);
    connect(actRefresh_, &QAction::triggered, this, &MainWindow::refresh);

    // Toolbar extra actions
    connect(actAddFile_, &QAction::triggered, this, &MainWindow::addFile);
    connect(actAddFolder_, &QAction::triggered, this, &MainWindow::addFolder);
    connect(actUp_, &QAction::triggered, this, &MainWindow::goUp);
    connect(actVerify_, &QAction::triggered, this, &MainWindow::verifyArchive);
    connect(actStats_, &QAction::triggered, this, &MainWindow::showStatistics);
    connect(actProperties_, &QAction::triggered, this, &MainWindow::showProperties);

    // Tree/List interaction
    connect(folderTree_, &QTreeView::clicked, this, &MainWindow::onTreeClicked);
    connect(entryTable_, &QTableView::doubleClicked, this, &MainWindow::onEntryActivated);
    connect(entryTable_, &QTableView::clicked, this, &MainWindow::onEntryClicked);
    connect(entryTable_, &QTableView::customContextMenuRequested,
            this, &MainWindow::onCustomContextMenu);

    // Address bar
    connect(addressEdit_, &QLineEdit::returnPressed, this, &MainWindow::onAddressReturnPressed);

    // Controller signals
    connect(controller_, &AppController::archiveOpened, this, &MainWindow::onArchiveOpened);
    connect(controller_, &AppController::archiveClosed, this, &MainWindow::onArchiveClosed);
    connect(controller_, &AppController::treeUpdated, this, &MainWindow::onTreeUpdated);
    connect(controller_, &AppController::verifyComplete, this, &MainWindow::onVerifyComplete);
}

void MainWindow::updateTitle() {
    if (controller_->isArchiveOpen()) {
        setWindowTitle(tr("LMDB Archive - %1").arg("Open"));
    } else {
        setWindowTitle(tr("LMDB Archive"));
    }
}

void MainWindow::updateStatusBar() {
    if (controller_->isArchiveOpen()) {
        auto stats = controller_->archive()->stats();
        statusLabel_->setText(tr("Files: %1 | Size: %2 | Dirs: %3")
            .arg(stats.total_files)
            .arg(stats.total_bytes)
            .arg(stats.dir_count));
    } else {
        statusLabel_->setText(tr("No archive open"));
    }
}

// --- Slots ---

void MainWindow::onArchiveOpened() {
    folderModel_->setRoot(controller_->treeRoot());
    previewPane_->setArchive(controller_->archive());

    currentPath_.clear();
    addressEdit_->clear();

    // Expand root in tree
    folderTree_->expandToDepth(0);

    updateEntryView({});
    updateTitle();
    updateStatusBar();

    // Enable actions
    for (auto* act : {actExtract_, actAddFile_, actAddFolder_, actDelete_,
                      actUp_, actRefresh_, actVerify_, actClose_, actStats_}) {
        if (act) act->setEnabled(true);
    }
}

void MainWindow::onArchiveClosed() {
    folderModel_->setRoot(nullptr);
    entryModel_->clear();
    previewPane_->clear();
    currentPath_.clear();
    addressEdit_->clear();
    updateTitle();
    statusLabel_->setText(tr("No archive open"));

    for (auto* act : {actExtract_, actAddFile_, actAddFolder_, actDelete_,
                      actUp_, actRefresh_, actVerify_, actStats_}) {
        if (act) act->setEnabled(false);
    }
}

void MainWindow::onTreeUpdated() {
    folderModel_->setRoot(controller_->treeRoot());
    updateEntryView(currentPath_);
    updateStatusBar();
}

void MainWindow::onVerifyComplete(const VerifyReport& report) {
    StatsDialog dlg(report, this);
    dlg.exec();
}

void MainWindow::openArchive() {
    QString path = QFileDialog::getExistingDirectory(this,
        tr("Open LMDB Archive"), {},
        QFileDialog::ShowDirsOnly);
    if (!path.isEmpty()) {
        controller_->openArchive(path);
    }
}

void MainWindow::createArchive() {
    QString sourceFolder = QFileDialog::getExistingDirectory(this,
        tr("Select Source Folder"));
    if (sourceFolder.isEmpty()) return;

    QString archivePath = QFileDialog::getSaveFileName(this,
        tr("Create Archive"), {},
        tr("LMDB Archive (*.lmdb)"));
    if (archivePath.isEmpty()) return;

    ProgressDialog dlg(tr("Creating Archive"), this);
    dlg.show();

    controller_->createArchive(sourceFolder, archivePath);
    dlg.setFinished(true, {});
    dlg.exec();
}

void MainWindow::closeArchive() {
    controller_->closeArchive();
}

void MainWindow::extractArchive() {
    QStringList selected;
    collectSelectedPaths(selected);

    ExtractDialog dlg(selected, this);
    if (dlg.exec() != QDialog::Accepted) return;

    QString outputFolder = dlg.outputFolder();
    if (outputFolder.isEmpty()) return;

    ProgressDialog progressDlg(tr("Extracting"), this);
    progressDlg.show();

    if (dlg.extractSelected() && !selected.isEmpty()) {
        controller_->extractSelection(selected, outputFolder);
    } else {
        controller_->extractAll(outputFolder);
    }

    progressDlg.setFinished(true, {});
    progressDlg.exec();
}

void MainWindow::addFile() {
    QStringList files = QFileDialog::getOpenFileNames(this, tr("Add Files"));
    if (files.isEmpty()) return;

    for (const auto& file : files) {
        QFileInfo fi(file);
        QString destPath = currentPath_.isEmpty()
            ? fi.fileName()
            : currentPath_ + QLatin1Char('/') + fi.fileName();
        controller_->addFile(file, destPath);
    }
}

void MainWindow::addFolder() {
    QString folder = QFileDialog::getExistingDirectory(this, tr("Add Folder"));
    if (folder.isEmpty()) return;

    controller_->addFolder(folder, currentPath_);
}

void MainWindow::deleteSelected() {
    QStringList selected;
    collectSelectedPaths(selected);

    if (selected.isEmpty()) return;

    int ret = QMessageBox::question(this, tr("Delete"),
        tr("Delete %1 item(s)?").arg(selected.size()),
        QMessageBox::Yes | QMessageBox::No);

    if (ret == QMessageBox::Yes) {
        controller_->deletePaths(selected);
    }
}

void MainWindow::goUp() {
    if (currentPath_.isEmpty()) return;
    QString parent = PathUtil::parentPath(currentPath_);
    navigateToPath(parent);
}

void MainWindow::refresh() {
    if (controller_->isArchiveOpen()) {
        updateEntryView(currentPath_);
    }
}

void MainWindow::verifyArchive() {
    controller_->verifyArchive();
}

void MainWindow::showProperties() {
    if (!controller_->isArchiveOpen()) return;

    auto stats = controller_->archive()->stats();
    auto manifest = controller_->archive()->manifest();
    PropertiesDialog dlg(manifest, stats, this);
    dlg.exec();
}

void MainWindow::showStatistics() {
    showProperties();
}

void MainWindow::onTreeClicked(const QModelIndex& index) {
    auto* node = folderModel_->nodeFromIndex(index);
    if (node && node->isDir()) {
        navigateToPath(node->fullPath());
    }
}

void MainWindow::onEntryActivated(const QModelIndex& index) {
    if (entryModel_->isDir(index.row())) {
        navigateToPath(entryModel_->fullPath(index.row()));
    }
}

void MainWindow::onEntryClicked(const QModelIndex& index) {
    if (!entryModel_->isDir(index.row())) {
        QString path = entryModel_->fullPath(index.row());
        previewPane_->showPreview(path);
    }
}

void MainWindow::onCustomContextMenu(const QPoint& point) {
    QModelIndex index = entryTable_->indexAt(point);
    if (!index.isValid()) return;

    QMenu menu(this);
    menu.addAction(tr("Open / Preview"), this, [this, index]() {
        if (entryModel_->isDir(index.row())) {
            navigateToPath(entryModel_->fullPath(index.row()));
        } else {
            previewPane_->showPreview(entryModel_->fullPath(index.row()));
        }
    });

    menu.addAction(tr("Extract..."), this, &MainWindow::extractArchive);
    menu.addSeparator();
    menu.addAction(tr("Add Files..."), this, &MainWindow::addFile);
    menu.addAction(tr("Add Folder..."), this, &MainWindow::addFolder);
    menu.addSeparator();
    menu.addAction(tr("Delete"), this, &MainWindow::deleteSelected);
    menu.addSeparator();
    menu.addAction(tr("Copy Path"), this, [this, index]() {
        QApplication::clipboard()->setText(entryModel_->fullPath(index.row()));
    });

    menu.exec(entryTable_->viewport()->mapToGlobal(point));
}

void MainWindow::onAddressReturnPressed() {
    navigateToPath(addressEdit_->text().trimmed());
}

void MainWindow::navigateToPath(const QString& path) {
    currentPath_ = path;
    addressEdit_->setText(path);
    updateEntryView(path);
}

void MainWindow::updateEntryView(const QString& dirPath) {
    if (!controller_->isArchiveOpen()) return;

    auto* root = controller_->treeRoot();
    if (!root) return;

    ArchiveTreeNode* node = findNode(dirPath);
    if (node) {
        entryModel_->setDirectoryNode(node);
    } else {
        entryModel_->clear();
    }
}

void MainWindow::collectSelectedPaths(QStringList& paths) {
    auto selection = entryTable_->selectionModel()->selectedRows();
    for (const auto& idx : selection) {
        paths.append(entryModel_->fullPath(idx.row()));
    }
}

ArchiveTreeNode* MainWindow::findNode(const QString& path) {
    auto* root = controller_->treeRoot();
    if (!root) return nullptr;

    if (path.isEmpty()) return root;

    ArchiveTreeNode* current = root;
    auto parts = PathUtil::splitArchivePath(path);

    for (const auto& part : parts) {
        current = current->findChild(part);
        if (!current) return nullptr;
    }
    return current;
}

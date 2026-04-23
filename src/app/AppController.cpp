#include "app/AppController.h"
#include "ui/MainWindow.h"
#include "model/ArchiveTreeBuilder.h"
#include <QMessageBox>
#include <spdlog/spdlog.h>

AppController::AppController(QObject* parent) : QObject(parent) {}

bool AppController::isArchiveOpen() const {
    return archive_.isOpen();
}

void AppController::openArchive(const QString& path) {
    closeArchive();

    if (!archive_.open(path, OpenMode::ReadWrite)) {
        // Try read-only
        if (!archive_.open(path, OpenMode::ReadOnly)) {
            QMessageBox::critical(nullptr, tr("Error"),
                tr("Cannot open archive:\n%1").arg(archive_.lastError()));
            return;
        }
    }

    rebuildTree();
    emit archiveOpened();
}

void AppController::createArchive(const QString& sourceFolder, const QString& archivePath) {
    closeArchive();

    if (!archive_.create(sourceFolder, archivePath)) {
        QMessageBox::critical(nullptr, tr("Error"),
            tr("Cannot create archive:\n%1").arg(archive_.lastError()));
        return;
    }

    rebuildTree();
    emit archiveOpened();
}

void AppController::closeArchive() {
    if (archive_.isOpen()) {
        archive_.close();
        treeRoot_.reset();
        emit archiveClosed();
    }
}

void AppController::extractAll(const QString& outputFolder) {
    std::atomic<bool> cancel{false};
    archive_.extractAll(outputFolder, nullptr, &cancel);
}

void AppController::extractSelection(const QStringList& paths, const QString& outputFolder) {
    std::atomic<bool> cancel{false};
    archive_.extractSelection(paths, outputFolder, nullptr, &cancel);
}

void AppController::addFile(const QString& filePath, const QString& destPath) {
    if (archive_.addFile(filePath, destPath)) {
        rebuildTree();
        emit treeUpdated();
    }
}

void AppController::addFolder(const QString& folderPath, const QString& destPrefix) {
    if (archive_.addFolder(folderPath, destPrefix)) {
        rebuildTree();
        emit treeUpdated();
    }
}

void AppController::deletePaths(const QStringList& paths) {
    for (const auto& p : paths) {
        archive_.removePath(p);
    }
    rebuildTree();
    emit treeUpdated();
}

void AppController::verifyArchive() {
    auto report = archive_.verify();
    emit verifyComplete(report);
}

void AppController::refreshTree() {
    if (archive_.isOpen()) {
        archive_.close();
        // Reopen and reload
        rebuildTree();
    }
}

void AppController::rebuildTree() {
    treeRoot_ = ArchiveTreeBuilder::build(archive_.entries());
}

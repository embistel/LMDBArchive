#pragma once
#include <QObject>
#include <memory>
#include "lmdb/LmdbArchive.h"
#include "model/ArchiveTreeNode.h"

class MainWindow;

class AppController : public QObject {
    Q_OBJECT
public:
    explicit AppController(QObject* parent = nullptr);

    void setMainWindow(MainWindow* window) { mainWindow_ = window; }
    LmdbArchive* archive() { return &archive_; }
    ArchiveTreeNode* treeRoot() const { return treeRoot_.get(); }

    // Actions
    void openArchive(const QString& path);
    void createArchive(const QString& sourceFolder, const QString& archivePath);
    void closeArchive();
    void extractAll(const QString& outputFolder);
    void extractSelection(const QStringList& paths, const QString& outputFolder);
    void addFile(const QString& filePath, const QString& destPath);
    void addFolder(const QString& folderPath, const QString& destPrefix);
    void deletePaths(const QStringList& paths);
    void verifyArchive();
    void refreshTree();

    bool isArchiveOpen() const;

signals:
    void archiveOpened();
    void archiveClosed();
    void treeUpdated();
    void verifyComplete(const VerifyReport& report);

private:
    void rebuildTree();

    MainWindow* mainWindow_ = nullptr;
    LmdbArchive archive_;
    std::unique_ptr<ArchiveTreeNode> treeRoot_;
};

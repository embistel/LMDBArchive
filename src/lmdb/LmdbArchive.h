#pragma once
#include "core/ArchiveTypes.h"
#include "core/ArchiveLock.h"
#include "lmdb/LmdbEnv.h"
#include <QString>
#include <QByteArray>
#include <QVector>
#include <functional>
#include <atomic>

// Progress callback: (completed, total, currentFilePath)
using ProgressCallback = std::function<void(int completed, int total, const QString& current)>;

class LmdbArchive {
public:
    LmdbArchive();
    ~LmdbArchive();

    // Create new archive from a folder
    bool create(const QString& sourceFolder, const QString& archivePath,
                ProgressCallback progress = nullptr,
                std::atomic<bool>* cancelFlag = nullptr);

    // Open existing archive
    bool open(const QString& archivePath, OpenMode mode = OpenMode::ReadOnly);
    void close();
    bool isOpen() const;

    // Read file content
    QByteArray readFile(const QString& archivePath);

    // List all entries with metadata
    QVector<ArchiveEntry> listEntries(const QString& prefix = {});

    // List all raw keys
    QStringList listKeys(const QString& prefix = {});

    // Add single file
    bool addFile(const QString& sourceFile, const QString& destPath);

    // Add folder recursively
    bool addFolder(const QString& sourceFolder, const QString& destPrefix = {},
                   ProgressCallback progress = nullptr,
                   std::atomic<bool>* cancelFlag = nullptr);

    // Extract all files
    bool extractAll(const QString& outputFolder,
                    ProgressCallback progress = nullptr,
                    std::atomic<bool>* cancelFlag = nullptr);

    // Extract selected files
    bool extractSelection(const QStringList& paths, const QString& outputFolder,
                          ProgressCallback progress = nullptr,
                          std::atomic<bool>* cancelFlag = nullptr);

    // Remove a file or folder (prefix-based)
    bool removePath(const QString& pathOrPrefix);

    // Verify archive integrity
    VerifyReport verify();

    // Get archive statistics
    ArchiveStats stats();

    // Get manifest
    ArchiveManifest manifest() const;

    // Get entries (cached after open)
    const QVector<ArchiveEntry>& entries() const { return entries_; }

    QString lastError() const { return lastError_; }
    LmdbEnv* env() { return &env_; }

private:
    bool writeEntry(MDB_txn* txn, MDB_dbi entriesDbi, MDB_dbi metaDbi,
                    const QString& path, const QByteArray& data, const ArchiveEntry& meta);
    bool deleteEntry(MDB_txn* txn, MDB_dbi entriesDbi, MDB_dbi metaDbi,
                     const QString& path);
    void updateManifest(MDB_txn* txn, MDB_dbi manifestDbi);
    void loadEntries();
    void ensureManifest(MDB_txn* txn, MDB_dbi manifestDbi);

    LmdbEnv env_;
    ArchiveLock archiveLock_;
    MDB_dbi entriesDbi_ = 0;
    MDB_dbi metaDbi_ = 0;
    MDB_dbi manifestDbi_ = 0;
    QVector<ArchiveEntry> entries_;
    ArchiveManifest manifest_;
    QString lastError_;
    bool writeMode_ = false;
};

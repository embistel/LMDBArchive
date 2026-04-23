#include "lmdb/LmdbArchive.h"
#include "core/PathUtil.h"
#include "core/FileSystemUtil.h"
#include "core/HashUtil.h"
#include "core/MetaCodec.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <spdlog/spdlog.h>

LmdbArchive::LmdbArchive() = default;

LmdbArchive::~LmdbArchive() {
    close();
}

bool LmdbArchive::create(const QString& sourceFolder, const QString& archivePath,
                          ProgressCallback progress, std::atomic<bool>* cancelFlag) {
    close();

    // Acquire write lock on .lmdb file
    if (!archiveLock_.lockForWrite(archivePath)) {
        lastError_ = archiveLock_.lastError();
        return false;
    }

    if (!env_.open(archivePath, false)) {
        lastError_ = env_.lastError();
        archiveLock_.unlock();
        return false;
    }
    writeMode_ = true;

    // Open DBIs
    MDB_txn* txn = env_.beginTxn(false);
    if (!txn) {
        lastError_ = env_.lastError();
        close();
        return false;
    }

    int rc = mdb_dbi_open(txn, "entries", MDB_CREATE, &entriesDbi_);
    if (rc != MDB_SUCCESS) {
        lastError_ = QStringLiteral("Failed to create entries DBI");
        env_.abortTxn(txn);
        close();
        return false;
    }

    rc = mdb_dbi_open(txn, "meta", MDB_CREATE, &metaDbi_);
    rc = mdb_dbi_open(txn, "manifest", MDB_CREATE, &manifestDbi_);

    // Scan source folder
    auto scanEntries = FileSystemUtil::scanDirectory(sourceFolder);
    int total = scanEntries.size();

    manifest_ = {};
    manifest_.created_at = QDateTime::currentDateTimeUtc();
    manifest_.updated_at = manifest_.created_at;
    manifest_.file_count = 0;
    manifest_.total_uncompressed_bytes = 0;

    QDir sourceDir(sourceFolder);

    for (int i = 0; i < total; ++i) {
        if (cancelFlag && cancelFlag->load()) {
            env_.abortTxn(txn);
            close();
            lastError_ = QStringLiteral("Operation cancelled");
            return false;
        }

        const auto& scanEntry = scanEntries[i];

        // Recalculate relative path from source root
        QString relPath = PathUtil::makeRelativePath(sourceFolder, scanEntry.absolute_path);
        if (!PathUtil::isValidArchivePath(relPath)) {
            spdlog::warn("Skipping invalid path: {}", relPath.toStdString());
            continue;
        }

        QFile file(scanEntry.absolute_path);
        if (!file.open(QIODevice::ReadOnly)) {
            spdlog::warn("Cannot read file: {}", scanEntry.absolute_path.toStdString());
            continue;
        }

        QByteArray data = file.readAll();
        file.close();

        QByteArray hash = HashUtil::computeHash(data);

        ArchiveEntry meta = MetaCodec::fromScanEntry(
            relPath, scanEntry.file_size, scanEntry.modified_time,
            scanEntry.created_time, hash);

        writeEntry(txn, entriesDbi_, metaDbi_, relPath, data, meta);

        manifest_.file_count++;
        manifest_.total_uncompressed_bytes += data.size();

        if (progress) progress(i + 1, total, relPath);
    }

    // Write manifest
    QByteArray manifestData = MetaCodec::encodeManifest(manifest_);
    MDB_val key{6, const_cast<char*>("_meta")};
    MDB_val val{static_cast<size_t>(manifestData.size()), manifestData.data()};
    mdb_put(txn, manifestDbi_, &key, &val, 0);

    env_.commitTxn(txn);

    // Reload entries for internal cache
    loadEntries();

    spdlog::info("Archive created: {} files, {} bytes",
                 manifest_.file_count, manifest_.total_uncompressed_bytes);
    return true;
}

bool LmdbArchive::open(const QString& archivePath, OpenMode mode) {
    close();

    bool readOnly = (mode == OpenMode::ReadOnly);

    // Acquire file lock
    if (readOnly) {
        if (!archiveLock_.lockForRead(archivePath)) {
            lastError_ = archiveLock_.lastError();
            return false;
        }
    } else {
        if (!archiveLock_.lockForWrite(archivePath)) {
            lastError_ = archiveLock_.lastError();
            return false;
        }
    }

    if (!env_.open(archivePath, readOnly)) {
        lastError_ = env_.lastError();
        archiveLock_.unlock();
        return false;
    }
    writeMode_ = !readOnly;

    // Open DBIs
    MDB_txn* txn = env_.beginTxn(true);
    if (!txn) {
        lastError_ = env_.lastError();
        close();
        return false;
    }

    mdb_dbi_open(txn, "entries", 0, &entriesDbi_);
    mdb_dbi_open(txn, "meta", 0, &metaDbi_);
    mdb_dbi_open(txn, "manifest", 0, &manifestDbi_);

    // Read manifest
    MDB_val key{6, const_cast<char*>("_meta")};
    MDB_val val;
    int rc = mdb_get(txn, manifestDbi_, &key, &val);
    if (rc == MDB_SUCCESS) {
        manifest_ = MetaCodec::decodeManifest(
            QByteArray(static_cast<const char*>(val.mv_data), val.mv_size));
    }

    env_.commitTxn(txn);

    // Load all entries into memory
    loadEntries();

    spdlog::info("Archive opened: {} entries", entries_.size());
    return true;
}

void LmdbArchive::close() {
    if (env_.isOpen()) {
        entries_.clear();
        manifest_ = {};
        env_.close();
    }
    archiveLock_.unlock();
}

bool LmdbArchive::isOpen() const {
    return env_.isOpen();
}

QByteArray LmdbArchive::readFile(const QString& archivePath) {
    QString normPath = PathUtil::normalizeArchivePath(archivePath);
    QByteArray result;

    MDB_txn* txn = env_.beginTxn(true);
    if (!txn) return {};

    MDB_val key{static_cast<size_t>(normPath.toUtf8().size()),
                const_cast<char*>(normPath.toUtf8().constData())};
    MDB_val val;

    int rc = mdb_get(txn, entriesDbi_, &key, &val);
    if (rc == MDB_SUCCESS) {
        result = QByteArray(static_cast<const char*>(val.mv_data), val.mv_size);
    } else {
        lastError_ = QStringLiteral("File not found: ") + normPath;
    }

    env_.commitTxn(txn);
    return result;
}

QVector<ArchiveEntry> LmdbArchive::listEntries(const QString& prefix) {
    QVector<ArchiveEntry> result;
    for (const auto& e : entries_) {
        if (prefix.isEmpty() || PathUtil::isUnderPrefix(e.relative_path, prefix)) {
            result.append(e);
        }
    }
    return result;
}

QStringList LmdbArchive::listKeys(const QString& prefix) {
    QStringList result;
    for (const auto& e : entries_) {
        if (prefix.isEmpty() || PathUtil::isUnderPrefix(e.relative_path, prefix)) {
            result.append(e.relative_path);
        }
    }
    return result;
}

bool LmdbArchive::addFile(const QString& sourceFile, const QString& destPath) {
    QString normPath = PathUtil::normalizeArchivePath(destPath);
    if (!PathUtil::isValidArchivePath(normPath)) {
        lastError_ = QStringLiteral("Invalid archive path: ") + normPath;
        return false;
    }

    QFile file(sourceFile);
    if (!file.open(QIODevice::ReadOnly)) {
        lastError_ = QStringLiteral("Cannot read file: ") + sourceFile;
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QFileInfo fi(sourceFile);
    QByteArray hash = HashUtil::computeHash(data);

    ArchiveEntry meta = MetaCodec::fromScanEntry(
        normPath, fi.size(), fi.lastModified().toUTC(),
        fi.birthTime().toUTC(), hash);

    MDB_txn* txn = env_.beginTxn(false);
    if (!txn) {
        lastError_ = env_.lastError();
        return false;
    }

    if (!writeEntry(txn, entriesDbi_, metaDbi_, normPath, data, meta)) {
        env_.abortTxn(txn);
        return false;
    }

    // Update manifest
    manifest_.file_count++;
    manifest_.total_uncompressed_bytes += data.size();
    manifest_.updated_at = QDateTime::currentDateTimeUtc();
    updateManifest(txn, manifestDbi_);

    if (env_.commitTxn(txn) != MDB_SUCCESS) {
        lastError_ = env_.lastError();
        return false;
    }

    // Update cache
    entries_.append(meta);

    spdlog::info("Added file: {}", normPath.toStdString());
    return true;
}

bool LmdbArchive::addFolder(const QString& sourceFolder, const QString& destPrefix,
                             ProgressCallback progress, std::atomic<bool>* cancelFlag) {
    auto scanEntries = FileSystemUtil::scanDirectory(sourceFolder);
    int total = scanEntries.size();

    for (int i = 0; i < total; ++i) {
        if (cancelFlag && cancelFlag->load()) {
            lastError_ = QStringLiteral("Operation cancelled");
            return false;
        }

        const auto& se = scanEntries[i];
        QString relPath = PathUtil::makeRelativePath(sourceFolder, se.absolute_path);

        if (!destPrefix.isEmpty()) {
            relPath = PathUtil::normalizeArchivePath(destPrefix + QLatin1Char('/') + relPath);
        }

        if (!PathUtil::isValidArchivePath(relPath)) continue;

        QFile file(se.absolute_path);
        if (!file.open(QIODevice::ReadOnly)) continue;
        QByteArray data = file.readAll();
        file.close();

        QByteArray hash = HashUtil::computeHash(data);
        ArchiveEntry meta = MetaCodec::fromScanEntry(
            relPath, se.file_size, se.modified_time, se.created_time, hash);

        MDB_txn* txn = env_.beginTxn(false);
        if (!txn) return false;

        writeEntry(txn, entriesDbi_, metaDbi_, relPath, data, meta);
        manifest_.file_count++;
        manifest_.total_uncompressed_bytes += data.size();
        updateManifest(txn, manifestDbi_);

        if (env_.commitTxn(txn) != MDB_SUCCESS) return false;

        entries_.append(meta);
        if (progress) progress(i + 1, total, relPath);
    }

    manifest_.updated_at = QDateTime::currentDateTimeUtc();
    spdlog::info("Added folder: {} files", total);
    return true;
}

bool LmdbArchive::extractAll(const QString& outputFolder,
                              ProgressCallback progress, std::atomic<bool>* cancelFlag) {
    return extractSelection(listKeys(), outputFolder, progress, cancelFlag);
}

bool LmdbArchive::extractSelection(const QStringList& paths, const QString& outputFolder,
                                    ProgressCallback progress, std::atomic<bool>* cancelFlag) {
    int total = paths.size();
    MDB_txn* txn = env_.beginTxn(true);
    if (!txn) {
        lastError_ = env_.lastError();
        return false;
    }

    for (int i = 0; i < total; ++i) {
        if (cancelFlag && cancelFlag->load()) {
            env_.abortTxn(txn);
            lastError_ = QStringLiteral("Operation cancelled");
            return false;
        }

        const QString& normPath = paths[i];
        QByteArray pathUtf8 = normPath.toUtf8();
        MDB_val key{static_cast<size_t>(pathUtf8.size()), pathUtf8.data()};
        MDB_val val;

        int rc = mdb_get(txn, entriesDbi_, &key, &val);
        if (rc != MDB_SUCCESS) {
            spdlog::warn("Entry not found during extraction: {}", normPath.toStdString());
            continue;
        }

        QString outPath = FileSystemUtil::buildExtractionPath(outputFolder, normPath);
        QFile file(outPath);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(static_cast<const char*>(val.mv_data), val.mv_size);
            file.close();
        }

        if (progress) progress(i + 1, total, normPath);
    }

    env_.commitTxn(txn);
    spdlog::info("Extracted {} files to {}", total, outputFolder.toStdString());
    return true;
}

bool LmdbArchive::removePath(const QString& pathOrPrefix) {
    QString norm = PathUtil::normalizeArchivePath(pathOrPrefix);
    QStringList toRemove;

    for (const auto& e : entries_) {
        if (e.relative_path == norm || PathUtil::isUnderPrefix(e.relative_path, norm)) {
            toRemove.append(e.relative_path);
        }
    }

    if (toRemove.isEmpty()) {
        lastError_ = QStringLiteral("Path not found: ") + norm;
        return false;
    }

    MDB_txn* txn = env_.beginTxn(false);
    if (!txn) return false;

    for (const auto& p : toRemove) {
        deleteEntry(txn, entriesDbi_, metaDbi_, p);
    }

    manifest_.file_count -= toRemove.size();
    manifest_.updated_at = QDateTime::currentDateTimeUtc();
    updateManifest(txn, manifestDbi_);

    if (env_.commitTxn(txn) != MDB_SUCCESS) return false;

    // Update cache
    entries_.erase(
        std::remove_if(entries_.begin(), entries_.end(),
            [&norm](const ArchiveEntry& e) {
                return e.relative_path == norm || PathUtil::isUnderPrefix(e.relative_path, norm);
            }),
        entries_.end());

    spdlog::info("Removed {} entries", toRemove.size());
    return true;
}

VerifyReport LmdbArchive::verify() {
    VerifyReport report;
    report.entries_checked = 0;
    report.meta_checked = 0;

    MDB_txn* txn = env_.beginTxn(true);
    if (!txn) {
        report.passed = false;
        report.errors << QStringLiteral("Cannot start read transaction");
        return report;
    }

    // Check manifest exists
    MDB_val mkey{6, const_cast<char*>("_meta")};
    MDB_val mval;
    if (mdb_get(txn, manifestDbi_, &mkey, &mval) != MDB_SUCCESS) {
        report.warnings << QStringLiteral("Manifest not found");
    }

    // Check all entries
    MDB_cursor* cursor;
    mdb_cursor_open(txn, entriesDbi_, &cursor);

    MDB_val key, val;
    int rc = mdb_cursor_get(cursor, &key, &val, MDB_FIRST);

    while (rc == MDB_SUCCESS) {
        QString path = QString::fromUtf8(
            static_cast<const char*>(key.mv_data), key.mv_size);
        report.entries_checked++;

        // Validate path
        if (!PathUtil::isValidArchivePath(path)) {
            report.errors << QStringLiteral("Invalid path: %1").arg(path);
            report.passed = false;
        }

        // Check meta exists
        MDB_val metaVal;
        QByteArray pathBytes = path.toUtf8();
        MDB_val pathKey{static_cast<size_t>(pathBytes.size()), pathBytes.data()};
        if (mdb_get(txn, metaDbi_, &pathKey, &metaVal) != MDB_SUCCESS) {
            report.warnings << QStringLiteral("Missing metadata for: %1").arg(path);
        } else {
            report.meta_checked++;
            QByteArray metaData(static_cast<const char*>(metaVal.mv_data), metaVal.mv_size);
            ArchiveEntry meta = MetaCodec::decode(path, metaData);

            // Check size match
            if (meta.file_size != static_cast<qint64>(val.mv_size)) {
                report.errors << QStringLiteral("Size mismatch for: %1 (meta=%2, actual=%3)")
                    .arg(path).arg(meta.file_size).arg(val.mv_size);
                report.passed = false;
            }
        }

        rc = mdb_cursor_get(cursor, &key, &val, MDB_NEXT);
    }

    mdb_cursor_close(cursor);
    env_.commitTxn(txn);

    spdlog::info("Verify complete: {} entries, {} meta, passed={}",
                 report.entries_checked, report.meta_checked, report.passed);
    return report;
}

ArchiveStats LmdbArchive::stats() {
    ArchiveStats s;
    s.total_files = entries_.size();
    s.total_bytes = 0;

    QSet<QString> dirs;
    for (const auto& e : entries_) {
        s.total_bytes += e.file_size;
        s.extension_distribution[e.extension]++;

        // Count virtual directories
        QString dir = PathUtil::dirPath(e.relative_path);
        while (!dir.isEmpty()) {
            dirs.insert(dir);
            dir = PathUtil::dirPath(dir);
        }
    }
    s.dir_count = dirs.size();
    return s;
}

ArchiveManifest LmdbArchive::manifest() const {
    return manifest_;
}

bool LmdbArchive::writeEntry(MDB_txn* txn, MDB_dbi entriesDbi, MDB_dbi metaDbi,
                              const QString& path, const QByteArray& data,
                              const ArchiveEntry& meta) {
    QByteArray pathBytes = path.toUtf8();
    MDB_val key{static_cast<size_t>(pathBytes.size()), pathBytes.data()};
    MDB_val val{static_cast<size_t>(data.size()), const_cast<char*>(data.constData())};

    int rc = mdb_put(txn, entriesDbi, &key, &val, 0);
    if (rc != MDB_SUCCESS) {
        spdlog::error("Failed to write entry: {} - {}", path.toStdString(), mdb_strerror(rc));
        return false;
    }

    // Write metadata
    QByteArray metaData = MetaCodec::encode(meta);
    MDB_val metaVal{static_cast<size_t>(metaData.size()), metaData.data()};
    rc = mdb_put(txn, metaDbi, &key, &metaVal, 0);
    if (rc != MDB_SUCCESS) {
        spdlog::error("Failed to write metadata: {}", path.toStdString());
        return false;
    }

    return true;
}

bool LmdbArchive::deleteEntry(MDB_txn* txn, MDB_dbi entriesDbi, MDB_dbi metaDbi,
                               const QString& path) {
    QByteArray pathBytes = path.toUtf8();
    MDB_val key{static_cast<size_t>(pathBytes.size()), pathBytes.data()};

    mdb_del(txn, entriesDbi, &key, nullptr);
    mdb_del(txn, metaDbi, &key, nullptr);
    return true;
}

void LmdbArchive::updateManifest(MDB_txn* txn, MDB_dbi manifestDbi) {
    QByteArray data = MetaCodec::encodeManifest(manifest_);
    MDB_val key{6, const_cast<char*>("_meta")};
    MDB_val val{static_cast<size_t>(data.size()), data.data()};
    mdb_put(txn, manifestDbi, &key, &val, 0);
}

void LmdbArchive::loadEntries() {
    entries_.clear();
    if (!env_.isOpen()) return;

    MDB_txn* txn = env_.beginTxn(true);
    if (!txn) return;

    MDB_cursor* cursor;
    mdb_cursor_open(txn, metaDbi_, &cursor);

    MDB_val key, val;
    int rc = mdb_cursor_get(cursor, &key, &val, MDB_FIRST);

    while (rc == MDB_SUCCESS) {
        QString path = QString::fromUtf8(
            static_cast<const char*>(key.mv_data), key.mv_size);
        QByteArray metaData(static_cast<const char*>(val.mv_data), val.mv_size);
        ArchiveEntry entry = MetaCodec::decode(path, metaData);
        entries_.append(std::move(entry));
        rc = mdb_cursor_get(cursor, &key, &val, MDB_NEXT);
    }

    mdb_cursor_close(cursor);
    env_.commitTxn(txn);
}

void LmdbArchive::ensureManifest(MDB_txn* txn, MDB_dbi manifestDbi) {
    MDB_val key{6, const_cast<char*>("_meta")};
    MDB_val val;
    if (mdb_get(txn, manifestDbi, &key, &val) != MDB_SUCCESS) {
        updateManifest(txn, manifestDbi);
    }
}

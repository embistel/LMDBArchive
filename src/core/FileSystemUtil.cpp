#include "core/FileSystemUtil.h"
#include "core/PathUtil.h"
#include <QDir>
#include <QFileInfo>
#include <QDateTime>

QVector<FileScanEntry> FileSystemUtil::scanDirectory(const QString& dirPath) {
    QVector<FileScanEntry> entries;
    QDir dir(dirPath);

    const auto infos = dir.entryInfoList(
        QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot,
        QDir::Name
    );

    for (const auto& info : infos) {
        if (info.isDir()) {
            auto sub = scanDirectory(info.absoluteFilePath());
            for (auto& e : sub) {
                QString relToRoot = dir.relativeFilePath(info.absoluteFilePath());
                e.relative_path = PathUtil::normalizeArchivePath(
                    relToRoot + QLatin1Char('/') + e.relative_path
                );
                entries.append(std::move(e));
            }
        } else if (info.isFile()) {
            FileScanEntry entry;
            entry.absolute_path = info.absoluteFilePath();
            entry.relative_path = PathUtil::normalizeArchivePath(info.fileName());
            entry.file_size = info.size();
            entry.modified_time = info.lastModified().toUTC();
            entry.created_time = info.birthTime().toUTC();
            entries.append(std::move(entry));
        }
    }
    return entries;
}

bool FileSystemUtil::ensureDirectory(const QString& dirPath) {
    QDir dir(dirPath);
    return dir.mkpath(QLatin1String("."));
}

QString FileSystemUtil::buildExtractionPath(const QString& outputDir, const QString& archivePath) {
    QString normalized = PathUtil::normalizeArchivePath(archivePath);
    // Replace / with platform separator
    QString localPath = normalized;
    localPath.replace(QLatin1Char('/'), QDir::separator());

    QString fullPath = outputDir + QDir::separator() + localPath;
    QString parentDir = QFileInfo(fullPath).absolutePath();
    ensureDirectory(parentDir);
    return fullPath;
}

#pragma once
#include <QString>
#include <QStringList>
#include <QVector>
#include <QDateTime>

struct FileScanEntry {
    QString absolute_path;
    QString relative_path;
    qint64 file_size;
    QDateTime modified_time;
    QDateTime created_time;
};

class FileSystemUtil {
public:
    // Recursively scan directory, collect regular files
    static QVector<FileScanEntry> scanDirectory(const QString& dirPath);

    // Ensure directory exists, create if needed
    static bool ensureDirectory(const QString& dirPath);

    // Build full extraction path and ensure parent dirs exist
    static QString buildExtractionPath(const QString& outputDir, const QString& archivePath);
};

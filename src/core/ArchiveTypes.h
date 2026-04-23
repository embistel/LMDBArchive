#pragma once
#include <QString>
#include <QByteArray>
#include <QDateTime>
#include <QVector>

struct ArchiveEntry {
    QString relative_path;
    qint64 file_size = 0;
    QDateTime modified_time_utc;
    QDateTime created_time_utc;
    QByteArray hash;       // xxhash64 as hex string
    QString extension;
    bool is_image = false;
    int image_width = 0;
    int image_height = 0;
};

struct ArchiveManifest {
    int format_version = 1;
    QDateTime created_at;
    QDateTime updated_at;
    QString tool_name = "LMDBArchive";
    QString tool_version = "1.0.0";
    qint64 file_count = 0;
    qint64 total_uncompressed_bytes = 0;
    QString comment;
};

struct ArchiveStats {
    qint64 total_files = 0;
    qint64 total_bytes = 0;
    QMap<QString, qint64> extension_distribution;
    int dir_count = 0;
};

struct VerifyReport {
    bool passed = true;
    QStringList errors;
    QStringList warnings;
    int entries_checked = 0;
    int meta_checked = 0;
};

enum class OverwritePolicy {
    Ask,
    Overwrite,
    Skip,
    Rename
};

enum class OpenMode {
    ReadOnly,
    ReadWrite
};

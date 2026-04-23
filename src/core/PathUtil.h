#pragma once
#include <QString>
#include <QStringList>

class PathUtil {
public:
    // Normalize path for archive internal key
    // - Convert \ to /
    // - Remove leading /
    // - Remove trailing /
    // - Collapse . and ..
    static QString normalizeArchivePath(const QString& path);

    // Split path by / separator
    static QStringList splitArchivePath(const QString& path);

    // Join path parts with /
    static QString joinArchivePath(const QStringList& parts);

    // Validate path for archive storage
    static bool isValidArchivePath(const QString& path);

    // Get parent directory path (empty for root-level)
    static QString parentPath(const QString& path);

    // Get filename from path
    static QString fileName(const QString& path);

    // Get directory part of a path (everything before last /)
    static QString dirPath(const QString& path);

    // Build relative path from base directory and absolute file path
    static QString makeRelativePath(const QString& baseDir, const QString& filePath);

    // Check if path is under a given prefix
    static bool isUnderPrefix(const QString& path, const QString& prefix);
};

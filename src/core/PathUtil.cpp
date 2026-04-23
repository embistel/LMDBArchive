#include "core/PathUtil.h"
#include <QDir>

QString PathUtil::normalizeArchivePath(const QString& path) {
    if (path.isEmpty()) return {};

    QString result = path;

    // Windows backslash to forward slash
    result.replace(QLatin1Char('\\'), QLatin1Char('/'));

    // Remove leading slashes
    while (result.startsWith(QLatin1Char('/')))
        result.remove(0, 1);

    // Remove trailing slashes
    while (result.endsWith(QLatin1Char('/')))
        result.chop(1);

    if (result.isEmpty()) return {};

    // Collapse . and ..
    QStringList parts = splitArchivePath(result);
    QStringList normalized;
    for (const auto& part : parts) {
        if (part == QLatin1Char('.'))
            continue;
        if (part == QLatin1String("..")) {
            if (!normalized.isEmpty())
                normalized.removeLast();
            continue;
        }
        normalized << part;
    }

    return normalized.join(QLatin1Char('/'));
}

QStringList PathUtil::splitArchivePath(const QString& path) {
    return path.split(QLatin1Char('/'), Qt::SkipEmptyParts);
}

QString PathUtil::joinArchivePath(const QStringList& parts) {
    return parts.join(QLatin1Char('/'));
}

bool PathUtil::isValidArchivePath(const QString& path) {
    if (path.isEmpty()) return false;
    if (path.startsWith(QLatin1Char('/'))) return false;
    if (path.contains(QLatin1String(".."))) return false;
    if (path.contains(QLatin1Char('\0'))) return false;

    QString normalized = normalizeArchivePath(path);
    if (normalized.isEmpty()) return false;

    // Check for path traversal after normalization
    auto parts = splitArchivePath(normalized);
    for (const auto& p : parts) {
        if (p == QLatin1String("..")) return false;
    }
    return true;
}

QString PathUtil::parentPath(const QString& path) {
    auto parts = splitArchivePath(path);
    if (parts.size() <= 1) return {};
    parts.removeLast();
    return parts.join(QLatin1Char('/'));
}

QString PathUtil::fileName(const QString& path) {
    auto parts = splitArchivePath(path);
    if (parts.isEmpty()) return {};
    return parts.last();
}

QString PathUtil::dirPath(const QString& path) {
    int idx = path.lastIndexOf(QLatin1Char('/'));
    if (idx < 0) return {};
    return path.left(idx);
}

QString PathUtil::makeRelativePath(const QString& baseDir, const QString& filePath) {
    QDir base(baseDir);
    QString rel = base.relativeFilePath(filePath);
    return normalizeArchivePath(rel);
}

bool PathUtil::isUnderPrefix(const QString& path, const QString& prefix) {
    if (prefix.isEmpty()) return true;
    QString normPrefix = normalizeArchivePath(prefix);
    if (!normPrefix.isEmpty()) normPrefix += QLatin1Char('/');
    return path.startsWith(normPrefix);
}

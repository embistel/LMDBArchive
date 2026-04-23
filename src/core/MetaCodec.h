#pragma once
#include "core/ArchiveTypes.h"
#include <QByteArray>
#include <QJsonObject>

class MetaCodec {
public:
    // Serialize ArchiveEntry metadata to JSON bytes
    static QByteArray encode(const ArchiveEntry& entry);

    // Deserialize JSON bytes to ArchiveEntry
    static ArchiveEntry decode(const QString& path, const QByteArray& data);

    // Serialize manifest to JSON bytes
    static QByteArray encodeManifest(const ArchiveManifest& manifest);

    // Deserialize manifest from JSON bytes
    static ArchiveManifest decodeManifest(const QByteArray& data);

    // Create ArchiveEntry from file scan result
    static ArchiveEntry fromScanEntry(const QString& relativePath, qint64 fileSize,
                                       const QDateTime& modifiedTime, const QDateTime& createdTime,
                                       const QByteArray& hash);

    // Determine if file is an image based on extension
    static bool isImageFile(const QString& extension);
};

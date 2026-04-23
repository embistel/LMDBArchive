#include "core/MetaCodec.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSet>

QByteArray MetaCodec::encode(const ArchiveEntry& entry) {
    QJsonObject obj;
    obj[QStringLiteral("path")] = entry.relative_path;
    obj[QStringLiteral("size")] = entry.file_size;
    obj[QStringLiteral("modified")] = entry.modified_time_utc.toString(Qt::ISODate);
    if (entry.created_time_utc.isValid())
        obj[QStringLiteral("created")] = entry.created_time_utc.toString(Qt::ISODate);
    obj[QStringLiteral("hash")] = QString::fromLatin1(entry.hash);
    obj[QStringLiteral("ext")] = entry.extension;
    if (entry.is_image) {
        obj[QStringLiteral("is_image")] = true;
        obj[QStringLiteral("img_w")] = entry.image_width;
        obj[QStringLiteral("img_h")] = entry.image_height;
    }
    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}

ArchiveEntry MetaCodec::decode(const QString& path, const QByteArray& data) {
    ArchiveEntry entry;
    entry.relative_path = path;

    QJsonParseError err;
    auto doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        return entry;
    }

    auto obj = doc.object();
    entry.file_size = obj[QStringLiteral("size")].toInteger(entry.file_size);
    entry.modified_time_utc = QDateTime::fromString(
        obj[QStringLiteral("modified")].toString(), Qt::ISODate);
    entry.created_time_utc = QDateTime::fromString(
        obj[QStringLiteral("created")].toString(), Qt::ISODate);
    entry.hash = obj[QStringLiteral("hash")].toString().toLatin1();
    entry.extension = obj[QStringLiteral("ext")].toString();
    entry.is_image = obj[QStringLiteral("is_image")].toBool(false);
    entry.image_width = obj[QStringLiteral("img_w")].toInt(0);
    entry.image_height = obj[QStringLiteral("img_h")].toInt(0);

    return entry;
}

QByteArray MetaCodec::encodeManifest(const ArchiveManifest& manifest) {
    QJsonObject obj;
    obj[QStringLiteral("format_version")] = manifest.format_version;
    obj[QStringLiteral("created_at")] = manifest.created_at.toString(Qt::ISODate);
    obj[QStringLiteral("updated_at")] = manifest.updated_at.toString(Qt::ISODate);
    obj[QStringLiteral("tool")] = manifest.tool_name;
    obj[QStringLiteral("tool_version")] = manifest.tool_version;
    obj[QStringLiteral("file_count")] = manifest.file_count;
    obj[QStringLiteral("total_bytes")] = manifest.total_uncompressed_bytes;
    if (!manifest.comment.isEmpty())
        obj[QStringLiteral("comment")] = manifest.comment;
    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}

ArchiveManifest MetaCodec::decodeManifest(const QByteArray& data) {
    ArchiveManifest manifest;

    QJsonParseError err;
    auto doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        return manifest;
    }

    auto obj = doc.object();
    manifest.format_version = obj[QStringLiteral("format_version")].toInt(1);
    manifest.created_at = QDateTime::fromString(
        obj[QStringLiteral("created_at")].toString(), Qt::ISODate);
    manifest.updated_at = QDateTime::fromString(
        obj[QStringLiteral("updated_at")].toString(), Qt::ISODate);
    manifest.tool_name = obj[QStringLiteral("tool")].toString();
    manifest.tool_version = obj[QStringLiteral("tool_version")].toString();
    manifest.file_count = obj[QStringLiteral("file_count")].toInteger(0);
    manifest.total_uncompressed_bytes = obj[QStringLiteral("total_bytes")].toInteger(0);
    manifest.comment = obj[QStringLiteral("comment")].toString();

    return manifest;
}

ArchiveEntry MetaCodec::fromScanEntry(const QString& relativePath, qint64 fileSize,
                                        const QDateTime& modifiedTime, const QDateTime& createdTime,
                                        const QByteArray& hash) {
    ArchiveEntry entry;
    entry.relative_path = relativePath;
    entry.file_size = fileSize;
    entry.modified_time_utc = modifiedTime;
    entry.created_time_utc = createdTime;
    entry.hash = hash;

    int dotIdx = relativePath.lastIndexOf(QLatin1Char('.'));
    if (dotIdx >= 0)
        entry.extension = relativePath.mid(dotIdx + 1).toLower();

    entry.is_image = isImageFile(entry.extension);

    return entry;
}

bool MetaCodec::isImageFile(const QString& extension) {
    static const QSet<QString> imageExts = {
        QStringLiteral("jpg"), QStringLiteral("jpeg"),
        QStringLiteral("png"), QStringLiteral("bmp"),
        QStringLiteral("gif"), QStringLiteral("webp"),
        QStringLiteral("tiff"), QStringLiteral("tif"),
        QStringLiteral("ico"), QStringLiteral("svg")
    };
    return imageExts.contains(extension.toLower());
}

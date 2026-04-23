#pragma once
#include <QByteArray>
#include <QString>

class HashUtil {
public:
    // Compute xxhash64 as hex string
    static QByteArray computeHash(const QByteArray& data);

    // Compute xxhash64 from file contents
    static QByteArray computeFileHash(const QString& filePath);
};

#include "core/HashUtil.h"
#include <xxhash.h>
#include <QFile>

QByteArray HashUtil::computeHash(const QByteArray& data) {
    XXH64_hash_t hash = XXH64(data.constData(), data.size(), 0);
    QByteArray result(16, '\0');
    for (int i = 0; i < 8; ++i) {
        quint8 byte = static_cast<quint8>((hash >> (i * 8)) & 0xFF);
        result[2 * i] = "0123456789abcdef"[byte >> 4];
        result[2 * i + 1] = "0123456789abcdef"[byte & 0x0F];
    }
    result.resize(16);
    return result;
}

QByteArray HashUtil::computeFileHash(const QString& filePath) {
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) return {};

    XXH64_state_t* state = XXH64_createState();
    XXH64_reset(state, 0);

    static constexpr int BUF_SIZE = 1024 * 1024; // 1MB
    QByteArray buf(BUF_SIZE, '\0');

    while (true) {
        qint64 n = f.read(buf.data(), BUF_SIZE);
        if (n <= 0) break;
        XXH64_update(state, buf.constData(), n);
    }

    XXH64_hash_t hash = XXH64_digest(state);
    XXH64_freeState(state);

    QByteArray result(16, '\0');
    for (int i = 0; i < 8; ++i) {
        quint8 byte = static_cast<quint8>((hash >> (i * 8)) & 0xFF);
        result[2 * i] = "0123456789abcdef"[byte >> 4];
        result[2 * i + 1] = "0123456789abcdef"[byte & 0x0F];
    }
    result.resize(16);
    return result;
}

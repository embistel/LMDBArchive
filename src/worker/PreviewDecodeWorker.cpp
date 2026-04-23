#include "worker/PreviewDecodeWorker.h"

PreviewDecodeWorker::PreviewDecodeWorker(QObject* parent)
    : QObject(parent) {}

void PreviewDecodeWorker::decode(const QByteArray& data, const QString& path) {
    QImage image;
    if (image.loadFromData(data)) {
        emit decoded(image, path);
    } else {
        emit failed(path);
    }
}

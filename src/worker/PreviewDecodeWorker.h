#pragma once
#include <QObject>
#include <QImage>
#include <QString>

class PreviewDecodeWorker : public QObject {
    Q_OBJECT
public:
    explicit PreviewDecodeWorker(QObject* parent = nullptr);

public slots:
    void decode(const QByteArray& data, const QString& path);

signals:
    void decoded(const QImage& image, const QString& path);
    void failed(const QString& path);
};

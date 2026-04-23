#pragma once
#include <QWidget>
#include <QImage>
#include <QLabel>
#include <QScrollArea>
#include <QThread>

class PreviewDecodeWorker;
class LmdbArchive;

class PreviewPane : public QWidget {
    Q_OBJECT
public:
    explicit PreviewPane(QWidget* parent = nullptr);
    ~PreviewPane();

    void setArchive(LmdbArchive* archive);
    void showPreview(const QString& path);
    void clear();

private slots:
    void onDecoded(const QImage& image, const QString& path);
    void onDecodeFailed(const QString& path);

private:
    LmdbArchive* archive_ = nullptr;
    QScrollArea* scrollArea_ = nullptr;
    QLabel* imageLabel_ = nullptr;
    QLabel* infoLabel_ = nullptr;

    QThread decodeThread_;
    PreviewDecodeWorker* decoder_ = nullptr;
    QString currentPath_;
};

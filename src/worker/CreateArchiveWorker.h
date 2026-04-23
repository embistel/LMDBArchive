#pragma once
#include <QObject>
#include <QString>
#include <atomic>

class LmdbArchive;

class CreateArchiveWorker : public QObject {
    Q_OBJECT
public:
    explicit CreateArchiveWorker(LmdbArchive* archive, QObject* parent = nullptr);

    void setParams(const QString& sourceFolder, const QString& archivePath);

public slots:
    void run();
    void cancel();

signals:
    void progress(int completed, int total, const QString& currentFile);
    void finished(bool success, const QString& error);

private:
    LmdbArchive* archive_;
    QString sourceFolder_;
    QString archivePath_;
    std::atomic<bool> cancelFlag_{false};
};

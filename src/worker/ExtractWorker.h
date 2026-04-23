#pragma once
#include <QObject>
#include <QString>
#include <QStringList>
#include <atomic>

class LmdbArchive;

class ExtractWorker : public QObject {
    Q_OBJECT
public:
    explicit ExtractWorker(LmdbArchive* archive, QObject* parent = nullptr);

    void setExtractAll(const QString& outputFolder);
    void setExtractSelection(const QStringList& paths, const QString& outputFolder);

public slots:
    void run();
    void cancel();

signals:
    void progress(int completed, int total, const QString& currentFile);
    void finished(bool success, const QString& error);

private:
    LmdbArchive* archive_;
    QString outputFolder_;
    QStringList selectedPaths_;
    bool extractAll_ = true;
    std::atomic<bool> cancelFlag_{false};
};

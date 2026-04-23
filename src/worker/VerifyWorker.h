#pragma once
#include <QObject>
#include "core/ArchiveTypes.h"

class LmdbArchive;

class VerifyWorker : public QObject {
    Q_OBJECT
public:
    explicit VerifyWorker(LmdbArchive* archive, QObject* parent = nullptr);

public slots:
    void run();

signals:
    void finished(const VerifyReport& report);

private:
    LmdbArchive* archive_;
};

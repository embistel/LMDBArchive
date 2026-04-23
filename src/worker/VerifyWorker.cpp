#include "worker/VerifyWorker.h"
#include "lmdb/LmdbArchive.h"

VerifyWorker::VerifyWorker(LmdbArchive* archive, QObject* parent)
    : QObject(parent), archive_(archive) {}

void VerifyWorker::run() {
    VerifyReport report = archive_->verify();
    emit finished(report);
}

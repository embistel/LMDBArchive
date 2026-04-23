#include "worker/CreateArchiveWorker.h"
#include "lmdb/LmdbArchive.h"

CreateArchiveWorker::CreateArchiveWorker(LmdbArchive* archive, QObject* parent)
    : QObject(parent), archive_(archive) {}

void CreateArchiveWorker::setParams(const QString& sourceFolder, const QString& archivePath) {
    sourceFolder_ = sourceFolder;
    archivePath_ = archivePath;
}

void CreateArchiveWorker::run() {
    cancelFlag_.store(false);

    bool ok = archive_->create(sourceFolder_, archivePath_,
        [this](int completed, int total, const QString& currentFile) {
            emit progress(completed, total, currentFile);
        }, &cancelFlag_);

    if (cancelFlag_.load()) {
        emit finished(false, QStringLiteral("Cancelled"));
    } else if (!ok) {
        emit finished(false, archive_->lastError());
    } else {
        emit finished(true, {});
    }
}

void CreateArchiveWorker::cancel() {
    cancelFlag_.store(true);
}

#include "worker/ExtractWorker.h"
#include "lmdb/LmdbArchive.h"

ExtractWorker::ExtractWorker(LmdbArchive* archive, QObject* parent)
    : QObject(parent), archive_(archive) {}

void ExtractWorker::setExtractAll(const QString& outputFolder) {
    outputFolder_ = outputFolder;
    extractAll_ = true;
    selectedPaths_.clear();
}

void ExtractWorker::setExtractSelection(const QStringList& paths, const QString& outputFolder) {
    outputFolder_ = outputFolder;
    selectedPaths_ = paths;
    extractAll_ = false;
}

void ExtractWorker::run() {
    cancelFlag_.store(false);

    auto cb = [this](int completed, int total, const QString& currentFile) {
        emit progress(completed, total, currentFile);
    };

    bool ok;
    if (extractAll_) {
        ok = archive_->extractAll(outputFolder_, cb, &cancelFlag_);
    } else {
        ok = archive_->extractSelection(selectedPaths_, outputFolder_, cb, &cancelFlag_);
    }

    if (cancelFlag_.load()) {
        emit finished(false, QStringLiteral("Cancelled"));
    } else if (!ok) {
        emit finished(false, archive_->lastError());
    } else {
        emit finished(true, {});
    }
}

void ExtractWorker::cancel() {
    cancelFlag_.store(true);
}

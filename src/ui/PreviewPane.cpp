#include "ui/PreviewPane.h"
#include "lmdb/LmdbArchive.h"
#include "worker/PreviewDecodeWorker.h"
#include "core/MetaCodec.h"
#include <QVBoxLayout>
#include <QPixmap>

PreviewPane::PreviewPane(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    scrollArea_ = new QScrollArea(this);
    scrollArea_->setWidgetResizable(true);
    scrollArea_->setAlignment(Qt::AlignCenter);

    imageLabel_ = new QLabel(this);
    imageLabel_->setAlignment(Qt::AlignCenter);
    imageLabel_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    imageLabel_->setText(tr("No preview"));
    scrollArea_->setWidget(imageLabel_);

    infoLabel_ = new QLabel(this);
    infoLabel_->setAlignment(Qt::AlignCenter);

    layout->addWidget(scrollArea_);
    layout->addWidget(infoLabel_);

    // Setup decode thread
    decodeThread_.start();
    decoder_ = new PreviewDecodeWorker();
    decoder_->moveToThread(&decodeThread_);

    connect(decoder_, &PreviewDecodeWorker::decoded, this, &PreviewPane::onDecoded);
    connect(decoder_, &PreviewDecodeWorker::failed, this, &PreviewPane::onDecodeFailed);
}

PreviewPane::~PreviewPane() {
    decodeThread_.quit();
    decodeThread_.wait();
    delete decoder_;
}

void PreviewPane::setArchive(LmdbArchive* archive) {
    archive_ = archive;
}

void PreviewPane::showPreview(const QString& path) {
    if (!archive_) return;

    currentPath_ = path;
    imageLabel_->setText(tr("Loading..."));
    infoLabel_->setText(path);

    QByteArray data = archive_->readFile(path);
    if (data.isEmpty()) {
        imageLabel_->setText(tr("Cannot read file"));
        return;
    }

    // Check if image
    QString ext = path.mid(path.lastIndexOf(QLatin1Char('.')) + 1).toLower();
    if (MetaCodec::isImageFile(ext)) {
        QMetaObject::invokeMethod(decoder_, "decode",
            Qt::QueuedConnection, Q_ARG(QByteArray, data), Q_ARG(QString, path));
    } else {
        imageLabel_->setText(tr("Preview not available\n(%1)").arg(ext.toUpper()));
        infoLabel_->setText(QStringLiteral("%1 (%2 bytes)")
            .arg(path).arg(data.size()));
    }
}

void PreviewPane::clear() {
    currentPath_.clear();
    imageLabel_->clear();
    imageLabel_->setText(tr("No preview"));
    infoLabel_->clear();
}

void PreviewPane::onDecoded(const QImage& image, const QString& path) {
    if (path != currentPath_) return; // outdated

    QPixmap pixmap = QPixmap::fromImage(image);
    // Scale to fit if too large
    if (pixmap.width() > scrollArea_->viewport()->width() ||
        pixmap.height() > scrollArea_->viewport()->height()) {
        pixmap = pixmap.scaled(scrollArea_->viewport()->size(),
            Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    imageLabel_->setPixmap(pixmap);
    infoLabel_->setText(QStringLiteral("%1 (%2x%3)")
        .arg(path).arg(image.width()).arg(image.height()));
}

void PreviewPane::onDecodeFailed(const QString& path) {
    if (path != currentPath_) return;
    imageLabel_->setText(tr("Cannot decode image"));
}

#include "ui/ProgressDialog.h"
#include <QVBoxLayout>

ProgressDialog::ProgressDialog(const QString& title, QWidget* parent)
    : QDialog(parent) {
    setWindowTitle(title);
    setMinimumWidth(400);
    setModal(true);

    auto* layout = new QVBoxLayout(this);

    statusLabel_ = new QLabel(tr("Preparing..."), this);
    layout->addWidget(statusLabel_);

    progressBar_ = new QProgressBar(this);
    progressBar_->setRange(0, 100);
    layout->addWidget(progressBar_);

    fileLabel_ = new QLabel(this);
    fileLabel_->setWordWrap(true);
    layout->addWidget(fileLabel_);

    cancelButton_ = new QPushButton(tr("Cancel"), this);
    layout->addWidget(cancelButton_, 0, Qt::AlignRight);

    connect(cancelButton_, &QPushButton::clicked, this, [this]() {
        cancelButton_->setEnabled(false);
        cancelButton_->setText(tr("Cancelling..."));
        emit cancelRequested();
    });
}

void ProgressDialog::setProgress(int completed, int total, const QString& currentFile) {
    if (total > 0) {
        progressBar_->setMaximum(total);
        progressBar_->setValue(completed);
        statusLabel_->setText(tr("Progress: %1 / %2").arg(completed).arg(total));
    }
    fileLabel_->setText(currentFile);
}

void ProgressDialog::setFinished(bool success, const QString& error) {
    if (success) {
        statusLabel_->setText(tr("Completed successfully."));
        progressBar_->setValue(progressBar_->maximum());
    } else {
        statusLabel_->setText(tr("Failed: %1").arg(error));
    }
    cancelButton_->setText(tr("Close"));
    cancelButton_->setEnabled(true);
    disconnect(cancelButton_, nullptr, this, nullptr);
    connect(cancelButton_, &QPushButton::clicked, this, &QDialog::accept);
}

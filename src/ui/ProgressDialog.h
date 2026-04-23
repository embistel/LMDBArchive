#pragma once
#include <QDialog>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>

class ProgressDialog : public QDialog {
    Q_OBJECT
public:
    explicit ProgressDialog(const QString& title, QWidget* parent = nullptr);

    void setProgress(int completed, int total, const QString& currentFile);
    void setFinished(bool success, const QString& error);

signals:
    void cancelRequested();

private:
    QProgressBar* progressBar_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QLabel* fileLabel_ = nullptr;
    QPushButton* cancelButton_ = nullptr;
};

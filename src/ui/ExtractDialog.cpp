#include "ui/ExtractDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFileDialog>
#include <QDialogButtonBox>

ExtractDialog::ExtractDialog(const QStringList& selectedPaths, QWidget* parent)
    : QDialog(parent), selectedPaths_(selectedPaths) {
    setWindowTitle(tr("Extract"));
    setMinimumWidth(400);

    auto* layout = new QVBoxLayout(this);

    // Scope
    layout->addWidget(new QLabel(tr("Extract scope:")));
    scopeCombo_ = new QComboBox(this);
    scopeCombo_->addItem(tr("All files"));
    if (!selectedPaths_.isEmpty()) {
        scopeCombo_->addItem(tr("Selected files (%1)").arg(selectedPaths_.size()));
        scopeCombo_->setCurrentIndex(1);
    }
    layout->addWidget(scopeCombo_);

    // Output folder
    layout->addWidget(new QLabel(tr("Output folder:")));
    auto* folderLayout = new QHBoxLayout();
    outputEdit_ = new QLineEdit(this);
    auto* browseBtn = new QPushButton(tr("Browse..."), this);
    folderLayout->addWidget(outputEdit_);
    folderLayout->addWidget(browseBtn);
    layout->addLayout(folderLayout);

    connect(browseBtn, &QPushButton::clicked, this, &ExtractDialog::browseOutputFolder);

    // Buttons
    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

QString ExtractDialog::outputFolder() const {
    return outputEdit_->text();
}

bool ExtractDialog::extractSelected() const {
    return scopeCombo_->currentIndex() == 1 && !selectedPaths_.isEmpty();
}

void ExtractDialog::browseOutputFolder() {
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select Output Folder"));
    if (!dir.isEmpty()) {
        outputEdit_->setText(dir);
    }
}

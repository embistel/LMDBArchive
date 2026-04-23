#include "ui/PropertiesDialog.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QLocale>

PropertiesDialog::PropertiesDialog(const ArchiveEntry& entry, QWidget* parent)
    : QDialog(parent) {
    setWindowTitle(tr("Properties: %1").arg(
        entry.relative_path.mid(entry.relative_path.lastIndexOf(QLatin1Char('/')) + 1)));
    setMinimumWidth(350);

    auto* layout = new QVBoxLayout(this);
    auto* form = new QFormLayout();

    form->addRow(tr("Path:"), new QLabel(entry.relative_path));
    form->addRow(tr("Size:"), new QLabel(
        QLocale().formattedDataSize(entry.file_size)));

    if (entry.modified_time_utc.isValid())
        form->addRow(tr("Modified:"), new QLabel(
            QLocale().toString(entry.modified_time_utc.toLocalTime(), QLocale::LongFormat)));

    if (!entry.extension.isEmpty())
        form->addRow(tr("Type:"), new QLabel(entry.extension.toUpper()));

    if (!entry.hash.isEmpty())
        form->addRow(tr("Hash:"), new QLabel(QString::fromLatin1(entry.hash)));

    if (entry.is_image) {
        form->addRow(tr("Image Size:"),
            new QLabel(QStringLiteral("%1 x %2").arg(entry.image_width).arg(entry.image_height)));
    }

    layout->addLayout(form);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok, this);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
}

PropertiesDialog::PropertiesDialog(const ArchiveManifest& manifest,
                                     const ArchiveStats& stats, QWidget* parent)
    : QDialog(parent) {
    setWindowTitle(tr("Archive Properties"));
    setMinimumWidth(400);

    auto* layout = new QVBoxLayout(this);

    auto* generalGroup = new QGroupBox(tr("General"), this);
    auto* form = new QFormLayout(generalGroup);

    form->addRow(tr("Format Version:"), new QLabel(QString::number(manifest.format_version)));
    form->addRow(tr("Tool:"), new QLabel(
        QStringLiteral("%1 v%2").arg(manifest.tool_name, manifest.tool_version)));
    form->addRow(tr("Created:"), new QLabel(
        QLocale().toString(manifest.created_at.toLocalTime(), QLocale::LongFormat)));
    form->addRow(tr("Updated:"), new QLabel(
        QLocale().toString(manifest.updated_at.toLocalTime(), QLocale::LongFormat)));

    layout->addWidget(generalGroup);

    auto* statsGroup = new QGroupBox(tr("Statistics"), this);
    auto* sForm = new QFormLayout(statsGroup);

    sForm->addRow(tr("Files:"), new QLabel(QString::number(stats.total_files)));
    sForm->addRow(tr("Total Size:"), new QLabel(
        QLocale().formattedDataSize(stats.total_bytes)));
    sForm->addRow(tr("Directories:"), new QLabel(QString::number(stats.dir_count)));

    layout->addWidget(statsGroup);

    if (!stats.extension_distribution.isEmpty()) {
        auto* extGroup = new QGroupBox(tr("Extension Distribution"), this);
        auto* extLayout = new QFormLayout(extGroup);
        for (auto it = stats.extension_distribution.constBegin();
             it != stats.extension_distribution.constEnd(); ++it) {
            extLayout->addRow(
                it.key().isEmpty() ? tr("(none)") : QStringLiteral("*.%1").arg(it.key()),
                new QLabel(QString::number(it.value())));
        }
        layout->addWidget(extGroup);
    }

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok, this);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
}

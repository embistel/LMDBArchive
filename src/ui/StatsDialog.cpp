#include "ui/StatsDialog.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QListWidget>

StatsDialog::StatsDialog(const VerifyReport& report, QWidget* parent)
    : QDialog(parent) {
    setWindowTitle(tr("Verify Results"));
    setMinimumWidth(450);

    auto* layout = new QVBoxLayout(this);

    // Summary
    auto* summaryGroup = new QGroupBox(tr("Summary"), this);
    auto* form = new QFormLayout(summaryGroup);

    form->addRow(tr("Entries checked:"), new QLabel(QString::number(report.entries_checked)));
    form->addRow(tr("Metadata checked:"), new QLabel(QString::number(report.meta_checked)));

    QString resultText = report.passed ? tr("PASSED") : tr("FAILED");
    auto* resultLabel = new QLabel(resultText, this);
    resultLabel->setStyleSheet(
        report.passed
            ? QStringLiteral("color: green; font-weight: bold;")
            : QStringLiteral("color: red; font-weight: bold;"));
    form->addRow(tr("Result:"), resultLabel);

    layout->addWidget(summaryGroup);

    // Errors
    if (!report.errors.isEmpty()) {
        auto* errGroup = new QGroupBox(tr("Errors"), this);
        auto* errList = new QListWidget(this);
        for (const auto& e : report.errors)
            errList->addItem(e);
        errGroup->setLayout(new QVBoxLayout);
        errGroup->layout()->addWidget(errList);
        layout->addWidget(errGroup);
    }

    // Warnings
    if (!report.warnings.isEmpty()) {
        auto* warnGroup = new QGroupBox(tr("Warnings"), this);
        auto* warnList = new QListWidget(this);
        for (const auto& w : report.warnings)
            warnList->addItem(w);
        warnGroup->setLayout(new QVBoxLayout);
        warnGroup->layout()->addWidget(warnList);
        layout->addWidget(warnGroup);
    }

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok, this);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
}

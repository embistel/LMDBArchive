#pragma once
#include <QDialog>
#include "core/ArchiveTypes.h"

class PropertiesDialog : public QDialog {
    Q_OBJECT
public:
    explicit PropertiesDialog(const ArchiveEntry& entry, QWidget* parent = nullptr);
    explicit PropertiesDialog(const ArchiveManifest& manifest,
                               const ArchiveStats& stats, QWidget* parent = nullptr);
};

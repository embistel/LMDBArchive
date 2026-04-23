#pragma once
#include <QDialog>
#include "core/ArchiveTypes.h"

class StatsDialog : public QDialog {
    Q_OBJECT
public:
    explicit StatsDialog(const VerifyReport& report, QWidget* parent = nullptr);
};

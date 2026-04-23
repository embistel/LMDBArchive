#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>

class ExtractDialog : public QDialog {
    Q_OBJECT
public:
    explicit ExtractDialog(const QStringList& selectedPaths, QWidget* parent = nullptr);

    QString outputFolder() const;
    bool extractSelected() const;

private slots:
    void browseOutputFolder();

private:
    QLineEdit* outputEdit_ = nullptr;
    QComboBox* scopeCombo_ = nullptr;
    QStringList selectedPaths_;
};

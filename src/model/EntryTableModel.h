#pragma once
#include <QAbstractTableModel>
#include <QVector>
#include "core/ArchiveTypes.h"

class ArchiveTreeNode;

class EntryTableModel : public QAbstractTableModel {
    Q_OBJECT
public:
    explicit EntryTableModel(QObject* parent = nullptr);

    void setEntries(const QVector<ArchiveEntry>& entries);
    void setDirectoryNode(ArchiveTreeNode* node);
    void clear();

    const ArchiveEntry& entry(int row) const;
    bool isDir(int row) const;
    QString name(int row) const;
    QString fullPath(int row) const;

    int rowCount(const QModelIndex& parent) const override;
    int columnCount(const QModelIndex& parent) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    enum Columns { ColName = 0, ColSize, ColType, ColModified, ColCount };

private:
    struct DisplayEntry {
        QString name;
        QString fullPath;
        bool isDir;
        qint64 fileSize;
        QString extension;
        QDateTime modified;
    };
    QVector<DisplayEntry> entries_;
};

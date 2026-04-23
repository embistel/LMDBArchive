#pragma once
#include <QAbstractItemModel>
#include "model/ArchiveTreeNode.h"

class FolderTreeModel : public QAbstractItemModel {
    Q_OBJECT
public:
    explicit FolderTreeModel(QObject* parent = nullptr);

    void setRoot(ArchiveTreeNode* root);
    ArchiveTreeNode* nodeFromIndex(const QModelIndex& index) const;

    QModelIndex index(int row, int column, const QModelIndex& parent) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    int rowCount(const QModelIndex& parent) const override;
    int columnCount(const QModelIndex& parent) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

private:
    ArchiveTreeNode* root_ = nullptr;
};

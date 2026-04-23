#include "model/FolderTreeModel.h"
#include <QIcon>

FolderTreeModel::FolderTreeModel(QObject* parent)
    : QAbstractItemModel(parent) {}

void FolderTreeModel::setRoot(ArchiveTreeNode* root) {
    beginResetModel();
    root_ = root;
    endResetModel();
}

ArchiveTreeNode* FolderTreeModel::nodeFromIndex(const QModelIndex& index) const {
    if (!index.isValid()) return root_;
    return static_cast<ArchiveTreeNode*>(index.internalPointer());
}

QModelIndex FolderTreeModel::index(int row, int column, const QModelIndex& parent) const {
    if (!root_) return {};

    ArchiveTreeNode* parentNode = nodeFromIndex(parent);
    if (!parentNode || row >= parentNode->childCount())
        return {};

    ArchiveTreeNode* childNode = parentNode->child(row);
    // Only show directories in the tree
    if (!childNode->isDir()) return {};

    return createIndex(row, column, childNode);
}

QModelIndex FolderTreeModel::parent(const QModelIndex& child) const {
    if (!child.isValid()) return {};

    ArchiveTreeNode* childNode = nodeFromIndex(child);
    if (!childNode) return {};

    ArchiveTreeNode* parentNode = childNode->parent();
    if (!parentNode || parentNode == root_) return {};

    return createIndex(parentNode->row(), 0, parentNode);
}

int FolderTreeModel::rowCount(const QModelIndex& parent) const {
    ArchiveTreeNode* node = nodeFromIndex(parent);
    if (!node) return 0;

    // Count only directory children
    int count = 0;
    for (auto* child : node->children()) {
        if (child->isDir()) count++;
    }
    return count;
}

int FolderTreeModel::columnCount(const QModelIndex&) const {
    return 1;
}

QVariant FolderTreeModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid()) return {};

    ArchiveTreeNode* node = nodeFromIndex(index);
    if (!node) return {};

    if (role == Qt::DisplayRole) {
        return node->name();
    }

    if (role == Qt::DecorationRole) {
        static QIcon folderIcon = QIcon::fromTheme(QStringLiteral("folder"));
        return folderIcon;
    }

    return {};
}

QVariant FolderTreeModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role == Qt::DisplayRole && section == 0)
        return tr("Name");
    return {};
}

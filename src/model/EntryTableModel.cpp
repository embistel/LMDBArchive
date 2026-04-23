#include "model/EntryTableModel.h"
#include "model/ArchiveTreeNode.h"
#include <QIcon>
#include <QApplication>
#include <QLocale>

EntryTableModel::EntryTableModel(QObject* parent)
    : QAbstractTableModel(parent) {}

void EntryTableModel::setEntries(const QVector<ArchiveEntry>& entries) {
    beginResetModel();
    entries_.clear();
    entries_.reserve(entries.size());

    for (const auto& e : entries) {
        DisplayEntry de;
        int lastSlash = e.relative_path.lastIndexOf(QLatin1Char('/'));
        de.name = lastSlash >= 0 ? e.relative_path.mid(lastSlash + 1) : e.relative_path;
        de.fullPath = e.relative_path;
        de.isDir = false;
        de.fileSize = e.file_size;
        de.extension = e.extension;
        de.modified = e.modified_time_utc;
        entries_.append(std::move(de));
    }

    endResetModel();
}

void EntryTableModel::setDirectoryNode(ArchiveTreeNode* node) {
    beginResetModel();
    entries_.clear();

    if (node) {
        for (auto* child : node->children()) {
            DisplayEntry de;
            de.name = child->name();
            de.fullPath = child->fullPath();
            de.isDir = child->isDir();

            if (child->isDir()) {
                de.fileSize = 0;
            } else {
                de.fileSize = child->entry().file_size;
                de.extension = child->entry().extension;
                de.modified = child->entry().modified_time_utc;
            }
            entries_.append(std::move(de));
        }
    }

    endResetModel();
}

void EntryTableModel::clear() {
    beginResetModel();
    entries_.clear();
    endResetModel();
}

const ArchiveEntry& EntryTableModel::entry(int row) const {
    static ArchiveEntry dummy;
    return dummy;
}

bool EntryTableModel::isDir(int row) const {
    if (row < 0 || row >= entries_.size()) return false;
    return entries_[row].isDir;
}

QString EntryTableModel::name(int row) const {
    if (row < 0 || row >= entries_.size()) return {};
    return entries_[row].name;
}

QString EntryTableModel::fullPath(int row) const {
    if (row < 0 || row >= entries_.size()) return {};
    return entries_[row].fullPath;
}

int EntryTableModel::rowCount(const QModelIndex&) const {
    return static_cast<int>(entries_.size());
}

int EntryTableModel::columnCount(const QModelIndex&) const {
    return ColCount;
}

QVariant EntryTableModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= entries_.size())
        return {};

    const auto& e = entries_[index.row()];

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case ColName:
            return e.name;
        case ColSize:
            if (e.isDir) return QStringLiteral("<DIR>");
            return QLocale().formattedDataSize(e.fileSize);
        case ColType:
            if (e.isDir) return tr("Folder");
            if (e.extension.isEmpty()) return tr("File");
            return e.extension.toUpper();
        case ColModified:
            if (!e.modified.isValid()) return {};
            return QLocale().toString(e.modified.toLocalTime(), QLocale::ShortFormat);
        }
    }

    if (role == Qt::DecorationRole && index.column() == ColName) {
        static QIcon folderIcon = QIcon::fromTheme(QStringLiteral("folder"));
        static QIcon fileIcon = QIcon::fromTheme(QStringLiteral("text-plain"));
        return e.isDir ? folderIcon : fileIcon;
    }

    if (role == Qt::TextAlignmentRole && index.column() == ColSize) {
        return Qt::AlignRight;
    }

    return {};
}

QVariant EntryTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return {};

    switch (section) {
    case ColName: return tr("Name");
    case ColSize: return tr("Size");
    case ColType: return tr("Type");
    case ColModified: return tr("Modified");
    }
    return {};
}

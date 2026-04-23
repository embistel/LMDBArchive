#pragma once
#include <QString>
#include <QList>
#include <memory>
#include "core/ArchiveTypes.h"

class ArchiveTreeNode {
public:
    explicit ArchiveTreeNode(const QString& name = {},
                             ArchiveTreeNode* parent = nullptr);
    ~ArchiveTreeNode();

    // Name (file or directory name)
    QString name() const { return name_; }
    void setName(const QString& name) { name_ = name; }

    // Full archive path
    QString fullPath() const;
    bool isDir() const { return isDir_; }
    void setIsDir(bool dir) { isDir_ = dir; }

    // Children management
    ArchiveTreeNode* child(int index) const;
    int childCount() const { return static_cast<int>(children_.size()); }
    int row() const;
    QList<ArchiveTreeNode*> children() const { return children_; }
    ArchiveTreeNode* parent() const { return parent_; }

    // Find or create child directory node
    ArchiveTreeNode* ensureDir(const QString& dirName);

    // Add file child
    ArchiveTreeNode* addFile(const QString& name, const ArchiveEntry& entry);

    // Find child by name
    ArchiveTreeNode* findChild(const QString& name) const;

    // Sort children: directories first, then files, alphabetically
    void sortChildren();

    // File metadata (valid for file nodes)
    ArchiveEntry entry() const { return entry_; }
    void setEntry(const ArchiveEntry& entry) { entry_ = entry; }

    // Remove all children
    void clear();

private:
    QString name_;
    ArchiveTreeNode* parent_;
    QList<ArchiveTreeNode*> children_;
    bool isDir_ = false;
    ArchiveEntry entry_;
};

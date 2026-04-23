#pragma once
#include "core/ArchiveTypes.h"
#include "model/ArchiveTreeNode.h"
#include <memory>

class ArchiveTreeBuilder {
public:
    // Build tree from list of archive entries
    static std::unique_ptr<ArchiveTreeNode> build(const QVector<ArchiveEntry>& entries);

    // Get direct children of a virtual directory
    static QVector<ArchiveEntry> listDirectory(const ArchiveTreeNode* root,
                                                const QString& dirPath);
};

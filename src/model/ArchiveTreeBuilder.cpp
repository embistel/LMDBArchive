#include "model/ArchiveTreeBuilder.h"
#include "core/PathUtil.h"

std::unique_ptr<ArchiveTreeNode> ArchiveTreeBuilder::build(const QVector<ArchiveEntry>& entries) {
    auto root = std::make_unique<ArchiveTreeNode>(QString()); // invisible root

    for (const auto& entry : entries) {
        QStringList parts = PathUtil::splitArchivePath(entry.relative_path);
        if (parts.isEmpty()) continue;

        ArchiveTreeNode* current = root.get();

        // Create/traverse directory nodes
        for (int i = 0; i < parts.size() - 1; ++i) {
            current = current->ensureDir(parts[i]);
        }

        // Add file node
        current->addFile(parts.last(), entry);
    }

    root->sortChildren();
    return root;
}

QVector<ArchiveEntry> ArchiveTreeBuilder::listDirectory(const ArchiveTreeNode* root,
                                                         const QString& dirPath) {
    QVector<ArchiveEntry> result;
    if (!root) return result;

    const ArchiveTreeNode* current = root;

    if (!dirPath.isEmpty()) {
        QStringList parts = PathUtil::splitArchivePath(dirPath);
        for (const auto& part : parts) {
            current = current->findChild(part);
            if (!current || !current->isDir()) return result;
        }
    }

    for (auto* child : current->children()) {
        if (child->isDir()) {
            ArchiveEntry dirEntry;
            dirEntry.relative_path = child->fullPath();
            dirEntry.is_image = false;
            result.append(dirEntry);
        } else {
            result.append(child->entry());
        }
    }

    return result;
}

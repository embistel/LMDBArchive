#include "model/ArchiveTreeNode.h"
#include <algorithm>

ArchiveTreeNode::ArchiveTreeNode(const QString& name, ArchiveTreeNode* parent)
    : name_(name), parent_(parent) {}

ArchiveTreeNode::~ArchiveTreeNode() {
    clear();
}

QString ArchiveTreeNode::fullPath() const {
    if (!parent_ || parent_->name().isEmpty()) return name_;
    return parent_->fullPath() + QLatin1Char('/') + name_;
}

ArchiveTreeNode* ArchiveTreeNode::child(int index) const {
    if (index < 0 || index >= children_.size()) return nullptr;
    return children_[index];
}

int ArchiveTreeNode::row() const {
    if (parent_)
        return parent_->children_.indexOf(const_cast<ArchiveTreeNode*>(this));
    return 0;
}

ArchiveTreeNode* ArchiveTreeNode::ensureDir(const QString& dirName) {
    auto* existing = findChild(dirName);
    if (existing && existing->isDir()) return existing;

    auto* node = new ArchiveTreeNode(dirName, this);
    node->setIsDir(true);
    children_.append(node);
    return node;
}

ArchiveTreeNode* ArchiveTreeNode::addFile(const QString& name, const ArchiveEntry& entry) {
    auto* node = new ArchiveTreeNode(name, this);
    node->setIsDir(false);
    node->setEntry(entry);
    children_.append(node);
    return node;
}

ArchiveTreeNode* ArchiveTreeNode::findChild(const QString& name) const {
    for (auto* child : children_) {
        if (child->name() == name) return child;
    }
    return nullptr;
}

void ArchiveTreeNode::sortChildren() {
    std::sort(children_.begin(), children_.end(),
        [](const ArchiveTreeNode* a, const ArchiveTreeNode* b) {
            if (a->isDir() != b->isDir()) return a->isDir();
            return a->name().toLower() < b->name().toLower();
        });

    for (auto* child : children_) {
        if (child->isDir()) child->sortChildren();
    }
}

void ArchiveTreeNode::clear() {
    qDeleteAll(children_);
    children_.clear();
}

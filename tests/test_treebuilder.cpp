#include <catch2/catch_test_macros.hpp>
#include "model/ArchiveTreeBuilder.h"
#include "core/ArchiveTypes.h"

TEST_CASE("TreeBuilder basic", "[tree]") {
    QVector<ArchiveEntry> entries;
    ArchiveEntry e1;
    e1.relative_path = "File.jpg";
    entries.append(e1);

    ArchiveEntry e2;
    e2.relative_path = "Sub/File2.jpg";
    entries.append(e2);

    ArchiveEntry e3;
    e3.relative_path = "Sub/File3.jpg";
    entries.append(e3);

    ArchiveEntry e4;
    e4.relative_path = "Other/File4.jpg";
    entries.append(e4);

    auto tree = ArchiveTreeBuilder::build(entries);
    REQUIRE(tree != nullptr);

    // Root should have 3 children: File.jpg, Sub/, Other/
    REQUIRE(tree->childCount() == 3);

    // Sub should have 2 files
    auto* sub = tree->findChild("Sub");
    REQUIRE(sub != nullptr);
    REQUIRE(sub->isDir());
    REQUIRE(sub->childCount() == 2);

    // Other should have 1 file
    auto* other = tree->findChild("Other");
    REQUIRE(other != nullptr);
    REQUIRE(other->isDir());
    REQUIRE(other->childCount() == 1);
}

TEST_CASE("TreeBuilder nested", "[tree]") {
    QVector<ArchiveEntry> entries;
    ArchiveEntry e1;
    e1.relative_path = "A/B/C.jpg";
    entries.append(e1);

    ArchiveEntry e2;
    e2.relative_path = "A/B/D.jpg";
    entries.append(e2);

    ArchiveEntry e3;
    e3.relative_path = "A/E/F.jpg";
    entries.append(e3);

    auto tree = ArchiveTreeBuilder::build(entries);
    REQUIRE(tree != nullptr);

    auto* a = tree->findChild("A");
    REQUIRE(a != nullptr);
    REQUIRE(a->isDir());
    REQUIRE(a->childCount() == 2); // B, E

    auto* b = a->findChild("B");
    REQUIRE(b != nullptr);
    REQUIRE(b->childCount() == 2); // C.jpg, D.jpg

    auto* e = a->findChild("E");
    REQUIRE(e != nullptr);
    REQUIRE(e->childCount() == 1); // F.jpg
}

TEST_CASE("TreeBuilder empty", "[tree]") {
    QVector<ArchiveEntry> entries;
    auto tree = ArchiveTreeBuilder::build(entries);
    REQUIRE(tree != nullptr);
    REQUIRE(tree->childCount() == 0);
}

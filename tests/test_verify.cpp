#include <catch2/catch_test_macros.hpp>
#include "lmdb/LmdbArchive.h"
#include <QTemporaryDir>
#include <QFile>

static bool createSimpleTestFiles(const QString& dir) {
    QDir d(dir);
    d.mkpath("Sub");

    QFile f1(dir + "/test.txt");
    if (!f1.open(QIODevice::WriteOnly)) return false;
    f1.write("hello world");
    f1.close();

    QFile f2(dir + "/Sub/nested.txt");
    if (!f2.open(QIODevice::WriteOnly)) return false;
    f2.write("nested content");
    f2.close();

    return true;
}

TEST_CASE("Verify valid archive", "[verify]") {
    QTemporaryDir srcDir;
    QTemporaryDir archiveDir;
    REQUIRE(srcDir.isValid());
    REQUIRE(archiveDir.isValid());

    createSimpleTestFiles(srcDir.path());
    QString archivePath = archiveDir.path() + "/test.lmdb";

    LmdbArchive archive;
    archive.create(srcDir.path(), archivePath);

    auto report = archive.verify();
    REQUIRE(report.passed);
    REQUIRE(report.entries_checked == 2);
    REQUIRE(report.meta_checked == 2);

    archive.close();
}

TEST_CASE("Verify after reopen", "[verify]") {
    QTemporaryDir srcDir;
    QTemporaryDir archiveDir;
    REQUIRE(srcDir.isValid());
    REQUIRE(archiveDir.isValid());

    createSimpleTestFiles(srcDir.path());
    QString archivePath = archiveDir.path() + "/test.lmdb";

    LmdbArchive archive;
    archive.create(srcDir.path(), archivePath);
    archive.close();

    REQUIRE(archive.open(archivePath, OpenMode::ReadOnly));
    auto report = archive.verify();
    REQUIRE(report.passed);
    REQUIRE(report.entries_checked == 2);

    archive.close();
}

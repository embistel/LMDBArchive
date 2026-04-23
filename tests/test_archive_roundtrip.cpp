#include <catch2/catch_test_macros.hpp>
#include "lmdb/LmdbArchive.h"
#include "core/FileSystemUtil.h"
#include <QDir>
#include <QFile>
#include <QTemporaryDir>

static bool createTestFiles(const QString& dir) {
    QDir d(dir);
    d.mkpath("Sub1/Sub2");

    QFile f1(dir + "/root.txt");
    if (!f1.open(QIODevice::WriteOnly)) return false;
    f1.write("root file content");
    f1.close();

    QFile f2(dir + "/Sub1/file1.txt");
    if (!f2.open(QIODevice::WriteOnly)) return false;
    f2.write("sub1 file content");
    f2.close();

    QFile f3(dir + "/Sub1/Sub2/file2.txt");
    if (!f3.open(QIODevice::WriteOnly)) return false;
    f3.write("sub2 file content");
    f3.close();

    return true;
}

TEST_CASE("Archive create and open", "[archive]") {
    QTemporaryDir srcDir;
    QTemporaryDir archiveDir;
    REQUIRE(srcDir.isValid());
    REQUIRE(archiveDir.isValid());

    REQUIRE(createTestFiles(srcDir.path()));

    QString archivePath = archiveDir.path() + "/test.lmdb";

    LmdbArchive archive;
    REQUIRE(archive.create(srcDir.path(), archivePath));
    REQUIRE(archive.isOpen());

    // Verify manifest
    auto m = archive.manifest();
    REQUIRE(m.file_count == 3);
    REQUIRE(m.format_version == 1);

    archive.close();
    REQUIRE(!archive.isOpen());
}

TEST_CASE("Archive roundtrip - create, close, reopen", "[archive]") {
    QTemporaryDir srcDir;
    QTemporaryDir archiveDir;
    QTemporaryDir extractDir;
    REQUIRE(srcDir.isValid());
    REQUIRE(archiveDir.isValid());
    REQUIRE(extractDir.isValid());

    createTestFiles(srcDir.path());
    QString archivePath = archiveDir.path() + "/test.lmdb";

    LmdbArchive archive;
    REQUIRE(archive.create(srcDir.path(), archivePath));
    archive.close();

    // Reopen
    REQUIRE(archive.open(archivePath, OpenMode::ReadOnly));
    REQUIRE(archive.entries().size() == 3);

    // Extract
    REQUIRE(archive.extractAll(extractDir.path()));
    archive.close();

    // Verify extracted files
    QFile f1(extractDir.path() + "/root.txt");
    REQUIRE(f1.open(QIODevice::ReadOnly));
    REQUIRE(f1.readAll() == "root file content");
    f1.close();

    QFile f2(extractDir.path() + "/Sub1/file1.txt");
    REQUIRE(f2.open(QIODevice::ReadOnly));
    REQUIRE(f2.readAll() == "sub1 file content");
    f2.close();

    QFile f3(extractDir.path() + "/Sub1/Sub2/file2.txt");
    REQUIRE(f3.open(QIODevice::ReadOnly));
    REQUIRE(f3.readAll() == "sub2 file content");
    f3.close();
}

TEST_CASE("Archive list entries", "[archive]") {
    QTemporaryDir srcDir;
    QTemporaryDir archiveDir;
    REQUIRE(srcDir.isValid());
    REQUIRE(archiveDir.isValid());

    createTestFiles(srcDir.path());
    QString archivePath = archiveDir.path() + "/test.lmdb";

    LmdbArchive archive;
    archive.create(srcDir.path(), archivePath);

    auto entries = archive.listEntries();
    REQUIRE(entries.size() == 3);

    auto keys = archive.listKeys();
    REQUIRE(keys.size() == 3);

    archive.close();
}

TEST_CASE("Archive read file", "[archive]") {
    QTemporaryDir srcDir;
    QTemporaryDir archiveDir;
    REQUIRE(srcDir.isValid());
    REQUIRE(archiveDir.isValid());

    createTestFiles(srcDir.path());
    QString archivePath = archiveDir.path() + "/test.lmdb";

    LmdbArchive archive;
    archive.create(srcDir.path(), archivePath);

    QByteArray data = archive.readFile("root.txt");
    REQUIRE(data == "root file content");

    data = archive.readFile("Sub1/Sub2/file2.txt");
    REQUIRE(data == "sub2 file content");

    archive.close();
}

TEST_CASE("Archive add and delete", "[archive]") {
    QTemporaryDir srcDir;
    QTemporaryDir archiveDir;
    REQUIRE(srcDir.isValid());
    REQUIRE(archiveDir.isValid());

    createTestFiles(srcDir.path());
    QString archivePath = archiveDir.path() + "/test.lmdb";

    LmdbArchive archive;
    archive.create(srcDir.path(), archivePath);

    // Add file
    QTemporaryDir addDir;
    QFile newFile(addDir.path() + "/new.txt");
    newFile.open(QIODevice::WriteOnly);
    newFile.write("new content");
    newFile.close();

    REQUIRE(archive.addFile(newFile.fileName(), "added/new.txt"));
    REQUIRE(archive.entries().size() == 4);

    // Read added file
    REQUIRE(archive.readFile("added/new.txt") == "new content");

    // Delete
    REQUIRE(archive.removePath("added/new.txt"));
    REQUIRE(archive.entries().size() == 3);

    archive.close();
}

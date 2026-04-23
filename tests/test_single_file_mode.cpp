#include <catch2/catch_test_macros.hpp>
#include "lmdb/LmdbArchive.h"
#include "core/ArchiveLock.h"
#include <QTemporaryDir>
#include <QFile>
#include <QFileInfo>

static bool createSimpleFiles(const QString& dir) {
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

TEST_CASE("Single file archive creation", "[single-file]") {
    QTemporaryDir srcDir;
    QTemporaryDir workDir;
    REQUIRE(srcDir.isValid());
    REQUIRE(workDir.isValid());

    createSimpleFiles(srcDir.path());
    QString archivePath = workDir.path() + "/test.lmdb";

    LmdbArchive archive;
    REQUIRE(archive.create(srcDir.path(), archivePath));

    // Verify the archive is a single file, not a directory
    QFileInfo info(archivePath);
    REQUIRE(info.exists());
    REQUIRE(info.isFile());
    REQUIRE(!info.isDir());
    REQUIRE(archivePath.endsWith(".lmdb"));

    // Verify contents
    REQUIRE(archive.entries().size() == 2);
    REQUIRE(archive.manifest().file_count == 2);

    archive.close();
}

TEST_CASE("Single file archive roundtrip", "[single-file]") {
    QTemporaryDir srcDir;
    QTemporaryDir workDir;
    QTemporaryDir extractDir;
    REQUIRE(srcDir.isValid());
    REQUIRE(workDir.isValid());
    REQUIRE(extractDir.isValid());

    createSimpleFiles(srcDir.path());
    QString archivePath = workDir.path() + "/test.lmdb";

    LmdbArchive archive;
    REQUIRE(archive.create(srcDir.path(), archivePath));
    archive.close();

    // Verify single file
    QFileInfo info(archivePath);
    REQUIRE(info.isFile());
    REQUIRE(info.size() > 0);

    // Reopen from single file
    REQUIRE(archive.open(archivePath, OpenMode::ReadOnly));
    REQUIRE(archive.entries().size() == 2);

    // Extract
    REQUIRE(archive.extractAll(extractDir.path()));
    archive.close();

    // Verify extracted files
    QFile f1(extractDir.path() + "/test.txt");
    REQUIRE(f1.open(QIODevice::ReadOnly));
    REQUIRE(f1.readAll() == "hello world");
    f1.close();

    QFile f2(extractDir.path() + "/Sub/nested.txt");
    REQUIRE(f2.open(QIODevice::ReadOnly));
    REQUIRE(f2.readAll() == "nested content");
    f2.close();
}

TEST_CASE("Single file archive verify", "[single-file]") {
    QTemporaryDir srcDir;
    QTemporaryDir workDir;
    REQUIRE(srcDir.isValid());
    REQUIRE(workDir.isValid());

    createSimpleFiles(srcDir.path());
    QString archivePath = workDir.path() + "/test.lmdb";

    LmdbArchive archive;
    archive.create(srcDir.path(), archivePath);

    auto report = archive.verify();
    REQUIRE(report.passed);
    REQUIRE(report.entries_checked == 2);

    archive.close();
}

TEST_CASE("Archive lock prevents double write", "[single-file]") {
    QTemporaryDir srcDir;
    QTemporaryDir workDir;
    REQUIRE(srcDir.isValid());
    REQUIRE(workDir.isValid());

    createSimpleFiles(srcDir.path());
    QString archivePath = workDir.path() + "/lock_test.lmdb";

    LmdbArchive archive1;
    REQUIRE(archive1.create(srcDir.path(), archivePath));

    // Try to open the same file for writing from another instance
    LmdbArchive archive2;
    bool opened = archive2.open(archivePath, OpenMode::ReadWrite);
    // Should fail because archive1 holds the write lock
    REQUIRE(!opened);

    archive1.close();

    // Now opening should succeed
    REQUIRE(archive2.open(archivePath, OpenMode::ReadOnly));
    archive2.close();
}

TEST_CASE("Archive lock standalone", "[single-file]") {
    QTemporaryDir workDir;
    REQUIRE(workDir.isValid());
    QString filePath = workDir.path() + "/lock_test.lmdb";

    // Create empty file
    QFile f(filePath);
    f.open(QIODevice::WriteOnly);
    f.close();

    ArchiveLock lock1;
    REQUIRE(lock1.lockForWrite(filePath));
    REQUIRE(lock1.isLocked());

    // Second lock should fail
    ArchiveLock lock2;
    REQUIRE(!lock2.lockForWrite(filePath));

    // Release first lock
    lock1.unlock();
    REQUIRE(!lock1.isLocked());

    // Now second should succeed
    REQUIRE(lock2.lockForWrite(filePath));
    lock2.unlock();
}

TEST_CASE("No sidecar files created", "[single-file]") {
    QTemporaryDir srcDir;
    QTemporaryDir workDir;
    REQUIRE(srcDir.isValid());
    REQUIRE(workDir.isValid());

    createSimpleFiles(srcDir.path());
    QString archivePath = workDir.path() + "/clean.lmdb";

    LmdbArchive archive;
    archive.create(srcDir.path(), archivePath);
    archive.close();

    // Check that only the .lmdb file exists in workDir (no lock files, no data.mdb)
    QDir dir(workDir.path());
    QStringList entries = dir.entryList(QDir::Files);
    REQUIRE(entries.size() == 1);
    REQUIRE(entries[0] == "clean.lmdb");
}

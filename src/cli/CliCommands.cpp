#include "cli/CliCommands.h"
#include "lmdb/LmdbArchive.h"
#include "core/MetaCodec.h"
#include <QCoreApplication>
#include <QTextStream>
#include <QFile>
#include <QFileInfo>
#include <iostream>
#include <spdlog/spdlog.h>

static QTextStream out(stdout);
static QTextStream err(stderr);

int CliCommands::create(LmdbArchive& archive, const QStringList& args) {
    if (args.size() < 2) {
        err << "Usage: lmdb-archive create <folder> <archive>\n";
        return 1;
    }

    QString folder = args[0];
    QString archivePath = args[1];

    std::atomic<bool> cancel{false};
    bool ok = archive.create(folder, archivePath,
        [](int completed, int total, const QString& file) {
            std::cout << "\r[" << completed << "/" << total << "] " << file.toStdString();
            std::cout.flush();
        }, &cancel);

    std::cout << "\n";
    if (!ok) {
        err << "Error: " << archive.lastError() << "\n";
        return 1;
    }

    out << "Archive created: " << archivePath << "\n";
    auto m = archive.manifest();
    out << "  Files: " << m.file_count << "\n";
    out << "  Total bytes: " << m.total_uncompressed_bytes << "\n";
    return 0;
}

int CliCommands::extract(LmdbArchive& archive, const QStringList& args) {
    if (args.size() < 2) {
        err << "Usage: lmdb-archive extract <archive> <output_folder>\n";
        return 1;
    }

    QString archivePath = args[0];
    QString outputFolder = args[1];

    if (!archive.open(archivePath, OpenMode::ReadOnly)) {
        err << "Error: " << archive.lastError() << "\n";
        return 1;
    }

    std::atomic<bool> cancel{false};
    bool ok = archive.extractAll(outputFolder,
        [](int completed, int total, const QString& file) {
            std::cout << "\r[" << completed << "/" << total << "] " << file.toStdString();
            std::cout.flush();
        }, &cancel);

    std::cout << "\n";
    if (!ok) {
        err << "Error: " << archive.lastError() << "\n";
        return 1;
    }

    out << "Extracted to: " << outputFolder << "\n";
    return 0;
}

int CliCommands::list(LmdbArchive& archive, const QStringList& args) {
    if (args.isEmpty()) {
        err << "Usage: lmdb-archive list <archive> [prefix]\n";
        return 1;
    }

    QString archivePath = args[0];
    QString prefix = args.size() > 1 ? args[1] : QString();

    if (!archive.open(archivePath, OpenMode::ReadOnly)) {
        err << "Error: " << archive.lastError() << "\n";
        return 1;
    }

    auto entries = archive.listEntries(prefix);
    for (const auto& e : entries) {
        out << e.relative_path;
        if (!e.extension.isEmpty())
            out << "\t" << e.file_size << "\t" << e.modified_time_utc.toString(Qt::ISODate);
        out << "\n";
    }

    out << "Total: " << entries.size() << " entries\n";
    return 0;
}

int CliCommands::verify(LmdbArchive& archive, const QStringList& args) {
    if (args.isEmpty()) {
        err << "Usage: lmdb-archive verify <archive>\n";
        return 1;
    }

    if (!archive.open(args[0], OpenMode::ReadOnly)) {
        err << "Error: " << archive.lastError() << "\n";
        return 1;
    }

    auto report = archive.verify();

    out << "Verify Results:\n";
    out << "  Entries checked: " << report.entries_checked << "\n";
    out << "  Metadata checked: " << report.meta_checked << "\n";

    if (!report.warnings.isEmpty()) {
        out << "  Warnings:\n";
        for (const auto& w : report.warnings)
            out << "    - " << w << "\n";
    }

    if (!report.errors.isEmpty()) {
        out << "  Errors:\n";
        for (const auto& e : report.errors)
            out << "    - " << e << "\n";
    }

    out << "  Result: " << (report.passed ? "PASSED" : "FAILED") << "\n";
    return report.passed ? 0 : 1;
}

int CliCommands::stat(LmdbArchive& archive, const QStringList& args) {
    if (args.isEmpty()) {
        err << "Usage: lmdb-archive stat <archive>\n";
        return 1;
    }

    if (!archive.open(args[0], OpenMode::ReadOnly)) {
        err << "Error: " << archive.lastError() << "\n";
        return 1;
    }

    auto s = archive.stats();
    auto m = archive.manifest();

    out << "Archive Statistics:\n";
    out << "  Format version: " << m.format_version << "\n";
    out << "  Created: " << m.created_at.toString(Qt::ISODate) << "\n";
    out << "  Updated: " << m.updated_at.toString(Qt::ISODate) << "\n";
    out << "  Tool: " << m.tool_name << " v" << m.tool_version << "\n";
    out << "  Total files: " << s.total_files << "\n";
    out << "  Total bytes: " << s.total_bytes << "\n";
    out << "  Directories: " << s.dir_count << "\n";

    if (!s.extension_distribution.isEmpty()) {
        out << "  Extension distribution:\n";
        for (auto it = s.extension_distribution.constBegin();
             it != s.extension_distribution.constEnd(); ++it) {
            out << "    ." << it.key() << ": " << it.value() << "\n";
        }
    }
    return 0;
}

int CliCommands::add(LmdbArchive& archive, const QStringList& args) {
    if (args.size() < 2) {
        err << "Usage: lmdb-archive add <archive> <file_or_folder> [dest_prefix]\n";
        return 1;
    }

    if (!archive.open(args[0], OpenMode::ReadWrite)) {
        err << "Error: " << archive.lastError() << "\n";
        return 1;
    }

    QString source = args[1];
    QString prefix = args.size() > 2 ? args[2] : QString();

    QFileInfo fi(source);
    bool ok;
    if (fi.isDir()) {
        ok = archive.addFolder(source, prefix);
    } else {
        QString destPath = prefix.isEmpty()
            ? fi.fileName()
            : prefix + QLatin1Char('/') + fi.fileName();
        ok = archive.addFile(source, destPath);
    }

    if (!ok) {
        err << "Error: " << archive.lastError() << "\n";
        return 1;
    }

    out << "Added: " << source << "\n";
    return 0;
}

int CliCommands::del(LmdbArchive& archive, const QStringList& args) {
    if (args.size() < 2) {
        err << "Usage: lmdb-archive delete <archive> <path_or_prefix>\n";
        return 1;
    }

    if (!archive.open(args[0], OpenMode::ReadWrite)) {
        err << "Error: " << archive.lastError() << "\n";
        return 1;
    }

    if (!archive.removePath(args[1])) {
        err << "Error: " << archive.lastError() << "\n";
        return 1;
    }

    out << "Deleted: " << args[1] << "\n";
    return 0;
}

int CliCommands::cat(LmdbArchive& archive, const QStringList& args) {
    if (args.size() < 3) {
        err << "Usage: lmdb-archive cat <archive> <internal_path> <output_file>\n";
        return 1;
    }

    if (!archive.open(args[0], OpenMode::ReadOnly)) {
        err << "Error: " << archive.lastError() << "\n";
        return 1;
    }

    QByteArray data = archive.readFile(args[1]);
    if (data.isEmpty()) {
        err << "Error: " << archive.lastError() << "\n";
        return 1;
    }

    QFile outFile(args[2]);
    if (!outFile.open(QIODevice::WriteOnly)) {
        err << "Error: Cannot write to " << args[2] << "\n";
        return 1;
    }

    outFile.write(data);
    outFile.close();

    out << "Extracted: " << args[1] << " -> " << args[2]
        << " (" << data.size() << " bytes)\n";
    return 0;
}

void CliCommands::printUsage() {
    out << "LMDB Archive Tool v1.0.0\n\n";
    out << "Usage: lmdb-archive <command> [args...]\n\n";
    out << "Commands:\n";
    out << "  create  <folder> <archive>           Create archive from folder\n";
    out << "  extract <archive> <output_folder>    Extract archive to folder\n";
    out << "  list    <archive> [prefix]           List archive contents\n";
    out << "  verify  <archive>                    Verify archive integrity\n";
    out << "  stat    <archive>                    Show archive statistics\n";
    out << "  add     <archive> <file_or_dir> [prefix]  Add files to archive\n";
    out << "  delete  <archive> <path>             Delete entry from archive\n";
    out << "  cat     <archive> <path> <output>    Extract single file\n";
}

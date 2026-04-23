#pragma once
#include <QString>
#include <QStringList>
#include "core/ArchiveTypes.h"

class LmdbArchive;

class CliCommands {
public:
    static int create(LmdbArchive& archive, const QStringList& args);
    static int extract(LmdbArchive& archive, const QStringList& args);
    static int list(LmdbArchive& archive, const QStringList& args);
    static int verify(LmdbArchive& archive, const QStringList& args);
    static int stat(LmdbArchive& archive, const QStringList& args);
    static int add(LmdbArchive& archive, const QStringList& args);
    static int del(LmdbArchive& archive, const QStringList& args);
    static int cat(LmdbArchive& archive, const QStringList& args);

    static void printUsage();
};

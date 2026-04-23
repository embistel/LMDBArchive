#include "cli/CliCommands.h"
#include "lmdb/LmdbArchive.h"
#include <QCoreApplication>
#include <QStringList>
#include <spdlog/spdlog.h>

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("LMDBArchive"));
    app.setApplicationVersion(QStringLiteral("1.0.0"));

    QStringList args = app.arguments();
    if (args.size() < 2) {
        CliCommands::printUsage();
        return 0;
    }

    QString command = args[1];
    QStringList cmdArgs = args.mid(2);

    spdlog::set_level(spdlog::level::warn);

    LmdbArchive archive;

    if (command == QLatin1String("create"))
        return CliCommands::create(archive, cmdArgs);
    if (command == QLatin1String("extract"))
        return CliCommands::extract(archive, cmdArgs);
    if (command == QLatin1String("list"))
        return CliCommands::list(archive, cmdArgs);
    if (command == QLatin1String("verify"))
        return CliCommands::verify(archive, cmdArgs);
    if (command == QLatin1String("stat"))
        return CliCommands::stat(archive, cmdArgs);
    if (command == QLatin1String("add"))
        return CliCommands::add(archive, cmdArgs);
    if (command == QLatin1String("delete"))
        return CliCommands::del(archive, cmdArgs);
    if (command == QLatin1String("cat"))
        return CliCommands::cat(archive, cmdArgs);

    CliCommands::printUsage();
    return 1;
}

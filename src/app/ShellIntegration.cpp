#include "app/ShellIntegration.h"

#include <QCoreApplication>
#include <QSettings>
#include <QFileInfo>

QString ShellIntegration::exePath() {
    return QCoreApplication::applicationFilePath();
}

bool ShellIntegration::registerFileAssociation() {
    const QString exe = exePath();

    // .lmdb extension -> ProgID
    QSettings ext(QStringLiteral(".lmdb"), QSettings::NativeFormat);
    ext.setValue(QStringLiteral("Default"), QStringLiteral("LMDBArchive"));
    ext.setValue(QStringLiteral("PerceivedType"), QStringLiteral("Archive"));
    ext.sync();
    if (ext.status() != QSettings::NoError) return false;

    // ProgID
    QSettings progId(QStringLiteral("LMDBArchive"), QSettings::NativeFormat);
    progId.setValue(QStringLiteral("Default"), QStringLiteral("LMDB Archive File"));
    progId.setValue(QStringLiteral("FriendlyTypeName"), QStringLiteral("LMDB Archive"));

    // DefaultIcon
    progId.setValue(QStringLiteral("DefaultIcon/Default"),
        QString("\"%1\",0").arg(exe));

    // shell/open/command
    progId.setValue(QStringLiteral("shell/open/command/Default"),
        QString("\"%1\" \"%2\"").arg(exe, "%1"));
    progId.sync();
    return progId.status() == QSettings::NoError;
}

bool ShellIntegration::unregisterFileAssociation() {
    QSettings ext(QStringLiteral(".lmdb"), QSettings::NativeFormat);
    ext.remove(QString());
    ext.sync();

    QSettings progId(QStringLiteral("LMDBArchive"), QSettings::NativeFormat);
    progId.remove(QString());
    progId.sync();
    return true;
}

bool ShellIntegration::registerContextMenu() {
    const QString exe = exePath();

    QSettings settings(QStringLiteral("Directory\\shell\\ArchiveToLMDB"),
                       QSettings::NativeFormat);
    settings.setValue(QStringLiteral("Default"), QStringLiteral("Archive to LMDB"));
    settings.setValue(QStringLiteral("Icon"),
        QString("\"%1\",0").arg(exe));
    settings.setValue(QStringLiteral("command/Default"),
        QString("\"%1\" --create-from-folder \"%2\"").arg(exe, "%1"));
    settings.sync();
    return settings.status() == QSettings::NoError;
}

bool ShellIntegration::unregisterContextMenu() {
    QSettings settings(QStringLiteral("Directory\\shell\\ArchiveToLMDB"),
                       QSettings::NativeFormat);
    settings.remove(QString());
    settings.sync();
    return true;
}

bool ShellIntegration::registerAll() {
    bool ok = registerFileAssociation();
    ok = registerContextMenu() && ok;
    return ok;
}

bool ShellIntegration::unregisterAll() {
    bool ok = unregisterFileAssociation();
    ok = unregisterContextMenu() && ok;
    return ok;
}

bool ShellIntegration::isLmdbFile(const QString& path) {
    return path.endsWith(QStringLiteral(".lmdb"), Qt::CaseInsensitive);
}

bool ShellIntegration::isDirectory(const QString& path) {
    return QFileInfo(path).isDir();
}

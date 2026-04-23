#pragma once
#include <QString>

class ShellIntegration {
public:
    static QString exePath();

    static bool registerFileAssociation();
    static bool unregisterFileAssociation();

    static bool registerContextMenu();
    static bool unregisterContextMenu();

    static bool registerAll();
    static bool unregisterAll();

    static bool isLmdbFile(const QString& path);
    static bool isDirectory(const QString& path);
};

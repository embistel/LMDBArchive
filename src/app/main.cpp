#include <QApplication>
#include <QMessageBox>
#include <QTimer>
#include "app/AppController.h"
#include "app/ShellIntegration.h"
#include "ui/MainWindow.h"
#include <spdlog/spdlog.h>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("LMDBArchive"));
    app.setApplicationVersion(QStringLiteral("1.0.0"));
    app.setOrganizationName(QStringLiteral("LMDBArchive"));

    spdlog::set_level(spdlog::level::warn);

    QStringList args = app.arguments();

    // Headless shell integration commands
    if (args.contains(QStringLiteral("--register-shell"))) {
        bool ok = ShellIntegration::registerAll();
        QMessageBox::information(nullptr, QStringLiteral("Shell Integration"),
            ok ? QStringLiteral("Shell integration registered successfully.")
               : QStringLiteral("Failed to register. Run as administrator."));
        return 0;
    }
    if (args.contains(QStringLiteral("--unregister-shell"))) {
        bool ok = ShellIntegration::unregisterAll();
        QMessageBox::information(nullptr, QStringLiteral("Shell Integration"),
            ok ? QStringLiteral("Shell integration removed successfully.")
               : QStringLiteral("Failed to unregister. Run as administrator."));
        return 0;
    }

    AppController controller;
    MainWindow window(&controller);
    controller.setMainWindow(&window);

    window.show();

    // Process file/folder arguments after event loop starts
    bool createFromFolder = args.contains(QStringLiteral("--create-from-folder"));
    QString pathArg;
    for (int i = 1; i < args.size(); ++i) {
        if (!args[i].startsWith(QLatin1Char('-'))) {
            pathArg = args[i];
            break;
        }
    }

    if (!pathArg.isEmpty()) {
        QTimer::singleShot(0, &window, [&window, &controller, pathArg, createFromFolder]() {
            if (createFromFolder && ShellIntegration::isDirectory(pathArg)) {
                window.handleCreateFromFolder(pathArg);
            } else if (ShellIntegration::isLmdbFile(pathArg)) {
                controller.openArchive(pathArg);
            }
        });
    }

    return app.exec();
}

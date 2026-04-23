#include <QApplication>
#include "app/AppController.h"
#include "ui/MainWindow.h"
#include <spdlog/spdlog.h>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("LMDBArchive"));
    app.setApplicationVersion(QStringLiteral("1.0.0"));
    app.setOrganizationName(QStringLiteral("LMDBArchive"));

    spdlog::set_level(spdlog::level::warn);

    AppController controller;
    MainWindow window(&controller);
    controller.setMainWindow(&window);

    window.show();
    return app.exec();
}

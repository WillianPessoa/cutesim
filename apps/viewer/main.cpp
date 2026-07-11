#include <csignal>

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QtQml/QQmlExtensionPlugin>

#include "ScenarioBridge.h"
#include "SimController.h"

Q_IMPORT_QML_PLUGIN(CuteSim_ViewerPlugin)

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName("CuteSim");
    QGuiApplication::setOrganizationName("cutesim");

    /* BUG-24: exit cleanly on SIGINT/SIGTERM so destructors run and the
       spawned rr-feedback dies with the viewer instead of lingering. */
    std::signal(SIGINT, [](int) { QCoreApplication::quit(); });
    std::signal(SIGTERM, [](int) { QCoreApplication::quit(); });

    SimController controller;
    ScenarioBridge scenarioBridge;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("controller", &controller);
    engine.rootContext()->setContextProperty("scenarioBridge", &scenarioBridge);
    engine.loadFromModule("CuteSim.Viewer", "Main");

    if (engine.rootObjects().isEmpty()) {
        return 1;
    }

    return QGuiApplication::exec();
}

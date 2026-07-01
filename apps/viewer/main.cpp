#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "SimController.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName("CuteSim");
    app.setOrganizationName("cutesim");

    SimController controller;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("controller", &controller);
    engine.loadFromModule("CuteSim.Viewer", "Main");

    if (engine.rootObjects().isEmpty())
        return 1;

    return app.exec();
}

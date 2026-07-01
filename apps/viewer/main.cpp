#include <QGuiApplication>
#include <QQmlApplicationEngine>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName("CuteSim");
    app.setOrganizationName("cutesim");

    QQmlApplicationEngine engine;
    engine.loadFromModule("CuteSim.Viewer", "Main");

    if (engine.rootObjects().isEmpty())
        return 1;

    return app.exec();
}

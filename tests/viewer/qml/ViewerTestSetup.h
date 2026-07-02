#pragma once

#include <QObject>
#include <QtQml/QQmlEngine>

class ViewerTestSetup : public QObject
{
    Q_OBJECT
public:
    Q_INVOKABLE void qmlEngineAvailable(QQmlEngine *engine) {
        engine->addImportPath(QStringLiteral(VIEWER_QML_BUILD_DIR));
    }
};

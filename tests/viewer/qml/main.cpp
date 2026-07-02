#include <QtQuickTest/quicktest.h>
#include <QtQml/QQmlExtensionPlugin>

#include "ViewerTestSetup.h"

/* Force-link the static QML plugin and its resource bundle. */
Q_IMPORT_QML_PLUGIN(CuteSim_ViewerPlugin)

QUICK_TEST_MAIN_WITH_SETUP(viewer, ViewerTestSetup)

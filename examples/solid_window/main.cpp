#include "vnm_qml_chrome/vnm_qml_chrome_runtime.h"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QUrl>

int main(int argc, char** argv)
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    if (!vnm_init_qml_chrome_runtime(engine)) {
        return 1;
    }

    const QUrl root_url(QStringLiteral("qrc:/examples/solid_window/main.qml"));
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreated,
        &app,
        [root_url](QObject* object, const QUrl& object_url) {
            if (!object && object_url == root_url) {
                QCoreApplication::exit(1);
            }
        },
        Qt::QueuedConnection);

    engine.load(root_url);
    return app.exec();
}

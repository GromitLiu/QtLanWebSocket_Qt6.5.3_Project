#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "WebSocketBackend.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName("Qt LAN WebSocket");

    WebSocketBackend backend;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("backend", &backend);
    engine.loadFromModule("QtLanWebSocket", "Main");

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}

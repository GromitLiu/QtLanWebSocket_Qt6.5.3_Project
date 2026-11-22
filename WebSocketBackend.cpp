#include "WebSocketBackend.h"

#include <QNetworkInterface>
#include <QFile>
#include <QDir>
#include <QCoreApplication>
#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QUrl>
#include <QUrlQuery>
#include <QGuiApplication>
#include <QClipboard>

WebSocketBackend::WebSocketBackend(QObject *parent)
    : QObject(parent),
      m_wsServer(QStringLiteral("Qt LAN WebSocket Server"),
                 QWebSocketServer::NonSecureMode, this)
{
    connect(&m_wsServer, &QWebSocketServer::newConnection,
            this, &WebSocketBackend::onNewConnection);
    connect(&m_httpServer, &QTcpServer::newConnection,
            this, &WebSocketBackend::handleHttpConnection);
    refreshAddresses();
}

WebSocketBackend::~WebSocketBackend() { stop(); }

bool WebSocketBackend::running() const { return m_running; }
QString WebSocketBackend::status() const { return m_status; }
QStringList WebSocketBackend::localAddresses() const { return m_addresses; }
int WebSocketBackend::clientCount() const { return m_clients.size(); }
QString WebSocketBackend::pageUrl() const {
    return m_addresses.isEmpty() ? QString() :
        QString("http://%1:%2").arg(m_addresses.first()).arg(m_httpPort);
}
QString WebSocketBackend::wsUrl() const {
    return m_addresses.isEmpty() ? QString() :
        QString("ws://%1:%2").arg(m_addresses.first()).arg(m_wsPort);
}
QVariantList WebSocketBackend::clients() const {
    QVariantList result;
    for (auto *client : m_clients) {
        QVariantMap item;
        item["address"] = client->peerAddress().toString();
        item["connectedAt"] = client->property("connectedAt").toString();
        result.append(item);
    }
    return result;
}

void WebSocketBackend::setStatus(const QString &value) {
    if (m_status == value) return;
    m_status = value;
    emit statusChanged();
}
void WebSocketBackend::log(const QString &message) {
    emit logMessage(QDateTime::currentDateTime().toString("HH:mm:ss  ") + message);
}

void WebSocketBackend::refreshAddresses() {
    QStringList found;
    const auto interfaces = QNetworkInterface::allInterfaces();
    for (const auto &iface : interfaces) {
        if (!(iface.flags() & QNetworkInterface::IsUp) ||
            (iface.flags() & QNetworkInterface::IsLoopBack)) continue;
        for (const auto &entry : iface.addressEntries()) {
            const QHostAddress ip = entry.ip();
            if (ip.protocol() == QAbstractSocket::IPv4Protocol &&
                ip != QHostAddress::LocalHost)
                found << ip.toString();
        }
    }
    found.removeDuplicates();
    m_addresses = found;
    emit localAddressesChanged();
    emit urlsChanged();
}

void WebSocketBackend::copyText(const QString &text) {
    if (QGuiApplication::clipboard()) QGuiApplication::clipboard()->setText(text);
}

void WebSocketBackend::start() {
    if (m_running) return;
    refreshAddresses();

    const bool wsOk = m_wsServer.listen(QHostAddress::AnyIPv4, m_wsPort);
    if (!wsOk) {
        setStatus("WebSocket 启动失败：" + m_wsServer.errorString());
        log(m_status);
        return;
    }

    const bool httpOk = m_httpServer.listen(QHostAddress::AnyIPv4, m_httpPort);
    if (!httpOk) {
        m_wsServer.close();
        setStatus("网页服务启动失败：" + m_httpServer.errorString());
        log(m_status);
        return;
    }

    m_running = true;
    emit runningChanged();
    setStatus("运行中：网页 8080 / WebSocket 8888");
    log("服务已启动，监听 0.0.0.0:8080 和 0.0.0.0:8888");
    if (m_addresses.isEmpty())
        log("未检测到 IPv4 地址，请检查网络连接");
    else
        for (const auto &ip : m_addresses)
            log(QString("访问网页：http://%1:%2").arg(ip).arg(m_httpPort));
}

void WebSocketBackend::stop() {
    if (!m_running && !m_wsServer.isListening() && !m_httpServer.isListening()) return;
    m_httpServer.close();
    m_wsServer.close();
    const auto clientsCopy = m_clients;
    for (auto *client : clientsCopy) {
        client->close();
        client->deleteLater();
    }
    m_clients.clear();
    m_running = false;
    emit runningChanged();
    emit clientsChanged();
    setStatus("已停止");
    log("服务已停止");
}

void WebSocketBackend::onNewConnection() {
    while (m_wsServer.hasPendingConnections()) {
        QWebSocket *client = m_wsServer.nextPendingConnection();
        if (!client) continue;
        client->setProperty("connectedAt", QDateTime::currentDateTime().toString("HH:mm:ss"));
        m_clients.append(client);
        connect(client, &QWebSocket::textMessageReceived,
                this, &WebSocketBackend::onTextMessage);
        connect(client, &QWebSocket::disconnected,
                this, &WebSocketBackend::onDisconnected);
        log("新设备连接：" + client->peerAddress().toString());
        emit clientsChanged();
        QJsonObject welcome{{"type","connected"},{"message","已连接 Qt 局域网服务端"}};
        client->sendTextMessage(QString::fromUtf8(QJsonDocument(welcome).toJson(QJsonDocument::Compact)));
    }
}

void WebSocketBackend::onTextMessage(const QString &message) {
    auto *client = qobject_cast<QWebSocket*>(sender());
    if (!client) return;
    log(QString("收到 [%1]：%2").arg(client->peerAddress().toString(), message));
    QJsonObject response{
        {"type", "message"},
        {"from", "server"},
        {"content", "电脑已收到：" + message},
        {"timestamp", QDateTime::currentDateTime().toString(Qt::ISODate)}
    };
    client->sendTextMessage(QString::fromUtf8(
        QJsonDocument(response).toJson(QJsonDocument::Compact)));
}

void WebSocketBackend::onDisconnected() {
    auto *client = qobject_cast<QWebSocket*>(sender());
    if (!client) return;
    log("设备断开：" + client->peerAddress().toString());
    m_clients.removeAll(client);
    client->deleteLater();
    emit clientsChanged();
}

void WebSocketBackend::broadcast(const QString &message) {
    QJsonObject obj{{"type","message"},{"from","server"},{"content",message},
                    {"timestamp",QDateTime::currentDateTime().toString(Qt::ISODate)}};
    sendToAll(QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact)));
}
void WebSocketBackend::sendToAll(const QString &jsonText) {
    for (auto *client : m_clients)
        if (client->state() == QAbstractSocket::ConnectedState)
            client->sendTextMessage(jsonText);
    log(QString("向 %1 台设备群发：%2").arg(m_clients.size()).arg(jsonText));
}

void WebSocketBackend::handleHttpConnection() {
    while (m_httpServer.hasPendingConnections()) {
        QTcpSocket *socket = m_httpServer.nextPendingConnection();
        connect(socket, &QTcpSocket::readyRead, socket, [this, socket]() {
            const QByteArray request = socket->readAll();
            const QList<QByteArray> firstLine = request.split('\n').value(0).trimmed().split(' ');
            const QByteArray path = firstLine.size() >= 2 ? firstLine.at(1) : "/";
            const QByteArray response = makeHttpResponse(path);
            socket->write(response);
            socket->disconnectFromHost();
        });
        connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
    }
}

QByteArray WebSocketBackend::makeHttpResponse(const QByteArray &rawPath) const {
    QByteArray path = rawPath.split('?').first();
    if (path == "/" || path.isEmpty()) path = "/index.html";
    if (path.contains("..") || path.contains('\\'))
        return "HTTP/1.1 400 Bad Request\r\nConnection: close\r\nContent-Length: 0\r\n\r\n";

    QString fileName = QString::fromUtf8(path.mid(1));
    if (fileName != "index.html" && fileName != "app.js" && fileName != "style.css")
        return "HTTP/1.1 404 Not Found\r\nConnection: close\r\nContent-Length: 0\r\n\r\n";

    QFile file(QDir(QCoreApplication::applicationDirPath()).filePath("web/" + fileName));
    if (!file.open(QIODevice::ReadOnly))
        return "HTTP/1.1 404 Not Found\r\nConnection: close\r\nContent-Length: 0\r\n\r\n";

    QByteArray body = file.readAll();
    QByteArray type = "text/plain; charset=utf-8";
    if (fileName.endsWith(".html")) type = "text/html; charset=utf-8";
    else if (fileName.endsWith(".js")) type = "application/javascript; charset=utf-8";
    else if (fileName.endsWith(".css")) type = "text/css; charset=utf-8";

    QByteArray header = "HTTP/1.1 200 OK\r\nContent-Type: " + type +
        "\r\nCache-Control: no-store\r\nConnection: close\r\nContent-Length: " +
        QByteArray::number(body.size()) + "\r\n\r\n";
    return header + body;
}

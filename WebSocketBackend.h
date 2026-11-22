#pragma once

#include <QObject>
#include <QWebSocketServer>
#include <QWebSocket>
#include <QTcpServer>
#include <QTcpSocket>
#include <QVariantList>
#include <QSet>

class WebSocketBackend : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(QStringList localAddresses READ localAddresses NOTIFY localAddressesChanged)
    Q_PROPERTY(int clientCount READ clientCount NOTIFY clientsChanged)
    Q_PROPERTY(QVariantList clients READ clients NOTIFY clientsChanged)
    Q_PROPERTY(QString pageUrl READ pageUrl NOTIFY urlsChanged)
    Q_PROPERTY(QString wsUrl READ wsUrl NOTIFY urlsChanged)

public:
    explicit WebSocketBackend(QObject *parent = nullptr);
    ~WebSocketBackend();

    bool running() const;
    QString status() const;
    QStringList localAddresses() const;
    int clientCount() const;
    QVariantList clients() const;
    QString pageUrl() const;
    QString wsUrl() const;

    Q_INVOKABLE void copyText(const QString &text);
    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void broadcast(const QString &message);
    Q_INVOKABLE void sendToAll(const QString &jsonText);

signals:
    void runningChanged();
    void statusChanged();
    void localAddressesChanged();
    void clientsChanged();
    void urlsChanged();
    void logMessage(const QString &message);

private slots:
    void onNewConnection();
    void onTextMessage(const QString &message);
    void onDisconnected();

private:
    void setStatus(const QString &value);
    void refreshAddresses();
    void handleHttpConnection();
    QByteArray makeHttpResponse(const QByteArray &path) const;
    void log(const QString &message);

    QWebSocketServer m_wsServer;
    QTcpServer m_httpServer;
    QList<QWebSocket*> m_clients;
    QStringList m_addresses;
    QString m_status = "未启动";
    bool m_running = false;
    quint16 m_wsPort = 8888;
    quint16 m_httpPort = 8080;
};

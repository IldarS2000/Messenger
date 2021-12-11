#ifndef CLIENT_CORE_H
#define CLIENT_CORE_H

#include <QObject>
#include <QSslSocket>
#include <QTcpSocket>
#include <QHostAddress>
#include <QJsonDocument>
#include "message.h"

class ClientCore : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(ClientCore)
public:
    explicit ClientCore(QObject* parent = nullptr);
    ~ClientCore() override;
public slots:
    void connectToServer(const QHostAddress& address, quint16 port);
    void login(const QString& username, const QString& password);
    void sendMessage(const QString& message, const QString& time);
    void disconnectFromHost();
private slots:
    void onReadyRead();
signals:
    void connected();
    void disconnected();
    void loggedIn();
    void loginError(const QString& reason);
    void messageReceived(const Message& message);
    void error(QAbstractSocket::SocketError socketError);
    void userJoined(const QString& username);
    void userLeft(const QString& username);
    void informJoiner(const QStringList& usernames, const QList<Message>& messages);

private:
    QSslSocket* clientSocket;
    QString name;
    bool logged;

private:
    void packetReceived(const QJsonObject& packet);
    void handleLoginPacket(const QJsonObject& packet);
    void handleMessagePacket(const QJsonObject& packet);
    void handleUserJoinedPacket(const QJsonObject& packet);
    void handleUserLeftPacket(const QJsonObject& packet);
    void handleInformJoinerPacket(const QJsonObject& packet);
    static bool isEqualPacketType(const QJsonValue& jsonType, const char* strType);
};

#endif // CLIENT_CORE_H

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
    void connectToServer(const QHostAddress& address, quint16 port);
    void login(const QString& username, const QString& password);
    void registerUser(const QString& username, const QString& password);
    void sendMessage(const QString& message, const QString& time);
    void disconnectFromHost();

private slots:
    void onReadyRead();
signals:
    void connectedSig();
    void disconnectedSig();
    void loggedInSig();
    void registeredSig();
    void loginErrorSig(const QString& reason);
    void registerErrorSig(const QString& reason);
    void messageReceivedSig(const Message& message);
    void errorSig(QAbstractSocket::SocketError socketError);
    void userJoinedSig(const QString& username);
    void userLeftSig(const QString& username);
    void informJoinerSig(const QStringList& usernames, const QList<Message>& messages);

private:
    QSslSocket* clientSocket;
    QString name;

private:
    void packetReceived(const QJsonObject& packet);
    void handleLoginPacket(const QJsonObject& packet);
    void handleRegisterPacket(const QJsonObject& packet);
    void handleMessagePacket(const QJsonObject& packet);
    void handleUserJoinedPacket(const QJsonObject& packet);
    void handleUserLeftPacket(const QJsonObject& packet);
    void handleInformJoinerPacket(const QJsonObject& packet);
    static bool isEqualPacketType(const QJsonValue& jsonType, const char* strType);
};

#endif // CLIENT_CORE_H

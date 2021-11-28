#ifndef CLIENT_CORE_H
#define CLIENT_CORE_H

#include <QObject>
#include <QTcpSocket>
#include <QHostAddress>
#include <QJsonDocument>

class ClientCore : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(ClientCore)
public:
    explicit ClientCore(QObject* parent = nullptr);
    ~ClientCore() override;
public slots:
    void connectToServer(const QHostAddress& address, quint16 port);
    void login(const QString& userName);
    void sendMessage(const QString& message);
    void disconnectFromHost();
private slots:
    void onReadyRead();
signals:
    void connected();
    void disconnected();
    void loggedIn();
    void loginError(const QString& reason);
    void messageReceived(const QString& sender, const QString& text);
    void error(QAbstractSocket::SocketError socketError);
    void userJoined(const QString& username);
    void userLeft(const QString& username);

private:
    QTcpSocket* clientSocket;
    bool logged;

private:
    void packetReceived(const QJsonObject& packet);
    void handleLoginPacket(const QJsonObject& packet);
    void handleMessagePacket(const QJsonObject& packet);
    void handleUserJoinedPacket(const QJsonObject& packet);
    void handleUserLeftPacket(const QJsonObject& packet);
    static bool isEqualPacketType(const QJsonValue& jsonType, const char* const strType);
};

#endif // CLIENT_CORE_H

#include <QTcpSocket>
#include <QDataStream>
#include <QJsonParseError>
#include <QJsonObject>
#include "clientcore.h"
#include "constants.h"

ClientCore::ClientCore(QObject* parent) : QObject(parent), clientSocket(new QTcpSocket(this)), logged(false)
{
    connect(clientSocket, &QTcpSocket::connected, this, &ClientCore::connected);
    connect(clientSocket, &QTcpSocket::disconnected, this, &ClientCore::disconnected);
    connect(clientSocket, &QTcpSocket::disconnected, this, [this]() -> void { logged = false; });

    connect(clientSocket, &QTcpSocket::readyRead, this, &ClientCore::onReadyRead);
    connect(clientSocket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this,
            &ClientCore::error);
}

ClientCore::~ClientCore()
{
    delete clientSocket;
}

void ClientCore::connectToServer(const QHostAddress& address, const quint16 port)
{
    clientSocket->connectToHost(address, port);
}

void ClientCore::login(const QString& userName)
{
    if (clientSocket->state() == QAbstractSocket::ConnectedState) {
        QDataStream clientStream(clientSocket);
        clientStream.setVersion(serializerVersion);

        QJsonObject packet;
        packet[Packet::Type::TYPE]     = Packet::Type::LOGIN;
        packet[Packet::Data::USERNAME] = userName;
        clientStream << QJsonDocument(packet).toJson(QJsonDocument::Compact);
    }
}

void ClientCore::sendMessage(const QString& message)
{
    QDataStream clientStream(clientSocket);
    clientStream.setVersion(serializerVersion);

    QJsonObject packet;
    packet[Packet::Type::TYPE] = Packet::Type::MESSAGE;
    packet[Packet::Data::TEXT] = message;
    clientStream << QJsonDocument(packet).toJson();
}

void ClientCore::disconnectFromHost()
{
    clientSocket->disconnectFromHost();
}

bool ClientCore::isEqualPacketType(const QJsonValue& jsonType, const char* const strType)
{
    return jsonType.toString().compare(QLatin1String(strType), Qt::CaseInsensitive) == 0;
}

void ClientCore::handleLoginPacket(const QJsonObject& packet)
{
    if (logged) {
        return;
    }
    const QJsonValue successVal = packet.value(QLatin1String(Packet::Data::SUCCESS));
    if (successVal.isNull() || !successVal.isBool()) {
        return;
    }
    const bool loginSuccess = successVal.toBool();
    if (loginSuccess) {
        emit loggedIn();
        return;
    }
    const QJsonValue reasonVal = packet.value(QLatin1String(Packet::Data::REASON));
    emit loginError(reasonVal.toString());
}

void ClientCore::handleMessagePacket(const QJsonObject& packet)
{
    const QJsonValue textVal = packet.value(QLatin1String(Packet::Data::TEXT));
    if (textVal.isNull() || !textVal.isString()) {
        return;
    }
    const QJsonValue senderVal = packet.value(QLatin1String(Packet::Data::SENDER));
    if (senderVal.isNull() || !senderVal.isString()) {
        return;
    }
    emit messageReceived(senderVal.toString(), textVal.toString());
}

void ClientCore::handleUserJoinedPacket(const QJsonObject& packet)
{
    const QJsonValue usernameVal = packet.value(QLatin1String(Packet::Data::USERNAME));
    if (usernameVal.isNull() || !usernameVal.isString()) {
        return;
    }
    emit userJoined(usernameVal.toString());
}

void ClientCore::handleUserLeftPacket(const QJsonObject& packet)
{
    const QJsonValue usernameVal = packet.value(QLatin1String(Packet::Data::USERNAME));
    if (usernameVal.isNull() || !usernameVal.isString()) {
        return;
    }
    emit userLeft(usernameVal.toString());
}

void ClientCore::packetReceived(const QJsonObject& packet)
{
    const QJsonValue packetTypeVal = packet.value(QLatin1String(Packet::Type::TYPE));
    if (packetTypeVal.isNull() || !packetTypeVal.isString()) {
        return;
    }

    if (isEqualPacketType(packetTypeVal, Packet::Type::LOGIN)) {
        handleLoginPacket(packet);
    } else if (isEqualPacketType(packetTypeVal, Packet::Type::MESSAGE)) {
        handleMessagePacket(packet);
    } else if (isEqualPacketType(packetTypeVal, Packet::Type::USER_JOINED)) {
        handleUserJoinedPacket(packet);
    } else if (isEqualPacketType(packetTypeVal, Packet::Type::USER_LEFT)) {
        handleUserLeftPacket(packet);
    }
}

void ClientCore::onReadyRead()
{
    QByteArray jsonData;
    QDataStream socketStream(clientSocket);
    socketStream.setVersion(serializerVersion);

    while (true) {
        socketStream.startTransaction();
        socketStream >> jsonData;
        if (socketStream.commitTransaction()) {
            QJsonParseError parseError  = {0};
            const QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData, &parseError);
            if (parseError.error == QJsonParseError::NoError) {
                if (jsonDoc.isObject()) {
                    packetReceived(jsonDoc.object());
                }
            }
        } else {
            break;
        }
    }
}

#include <QTcpSocket>
#include <QDataStream>
#include <QJsonParseError>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include "clientcore.h"
#include "constants.h"

ClientCore::ClientCore(QObject* parent) : QObject(parent), clientSocket(new QSslSocket(this))
{
#ifdef SSL_ENABLE
    clientSocket->setProtocol(QSsl::SslV3);
    QByteArray certificate;
    QFile fileCertificate("ssl/ssl.cert");
    if (fileCertificate.open(QIODevice::ReadOnly)) {
        certificate = fileCertificate.readAll();
        fileCertificate.close();
    } else {
        qWarning() << fileCertificate.errorString();
    }
    QSslCertificate sslCertificate(certificate);
    clientSocket->setLocalCertificate(sslCertificate);
#endif

    connect(clientSocket, &QSslSocket::connected, this, &ClientCore::connected);
    connect(clientSocket, &QSslSocket::disconnected, this, &ClientCore::disconnected);

    connect(clientSocket, &QSslSocket::readyRead, this, &ClientCore::onReadyRead);
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
#ifdef SSL_ENABLE
    clientSocket->startClientEncryption();
#endif
}

void ClientCore::login(const QString& username, const QString& password)
{
    if (clientSocket->state() == QAbstractSocket::ConnectedState) {
        this->name = username;
        QDataStream clientStream(clientSocket);
        clientStream.setVersion(serializerVersion);

        QJsonObject packet;
        packet[Packet::Type::TYPE]     = Packet::Type::LOGIN;
        packet[Packet::Data::USERNAME] = username;
        packet[Packet::Data::PASSWORD] = password;
        clientStream << QJsonDocument(packet).toJson(QJsonDocument::Compact);
    }
}

void ClientCore::sendMessage(const QString& message, const QString& time)
{
    QDataStream clientStream(clientSocket);
    clientStream.setVersion(serializerVersion);

    QJsonObject packet;
    packet[Packet::Type::TYPE]   = Packet::Type::MESSAGE;
    packet[Packet::Data::SENDER] = name;
    packet[Packet::Data::TEXT]   = message;
    packet[Packet::Data::TIME]   = time;
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
    const QJsonValue senderVal = packet.value(QLatin1String(Packet::Data::SENDER));
    if (senderVal.isNull() || !senderVal.isString()) {
        return;
    }
    const QJsonValue textVal = packet.value(QLatin1String(Packet::Data::TEXT));
    if (textVal.isNull() || !textVal.isString()) {
        return;
    }
    const QJsonValue timeVal = packet.value(QLatin1String(Packet::Data::TIME));
    if (timeVal.isNull() || !timeVal.isString()) {
        return;
    }
    emit messageReceived({senderVal.toString(), textVal.toString(), timeVal.toString()});
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

void ClientCore::handleInformJoinerPacket(const QJsonObject& packet)
{
    const QJsonValue usernamesVal = packet.value(QLatin1String(Packet::Data::USERNAMES));
    if (usernamesVal.isNull() || !usernamesVal.isArray()) {
        return;
    }
    QJsonArray jsonUsernames = usernamesVal.toArray();
    QStringList usernames;
    for (const auto& jsonUsername : jsonUsernames) {
        usernames.push_back(jsonUsername.toString());
    }

    const QJsonValue messagesVal = packet.value(QLatin1String(Packet::Data::MESSAGES));
    if (messagesVal.isNull() || !messagesVal.isArray()) {
        return;
    }

    QJsonArray jsonMessages = messagesVal.toArray();
    QList<Message> messages;
    for (const auto& jsonMessage : jsonMessages) {
        QJsonObject obj = jsonMessage.toObject();
        QString sender  = obj[Packet::Data::SENDER].toString();
        QString text    = obj[Packet::Data::TEXT].toString();
        QString time    = obj[Packet::Data::TIME].toString();
        messages.push_back({sender, text, time});
    }
    emit informJoiner(usernames, messages);
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
    } else if (isEqualPacketType(packetTypeVal, Packet::Type::INFORM_JOINER)) {
        handleInformJoinerPacket(packet);
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

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

        QJsonObject dataUnit;
        dataUnit[Packet::Type::TYPE]     = Packet::Type::LOGIN;
        dataUnit[Packet::Data::USERNAME] = userName;
        clientStream << QJsonDocument(dataUnit).toJson(QJsonDocument::Compact);
    }
}

void ClientCore::sendMessage(const QString& message)
{
    if (message.isEmpty()) {
        return;
    }
    QDataStream clientStream(clientSocket);
    clientStream.setVersion(serializerVersion);

    QJsonObject dataUnit;
    dataUnit[Packet::Type::TYPE] = Packet::Type::MESSAGE;
    dataUnit[Packet::Data::TEXT] = message;
    clientStream << QJsonDocument(dataUnit).toJson();
}

void ClientCore::disconnectFromHost()
{
    clientSocket->disconnectFromHost();
}

void ClientCore::jsonReceived(const QJsonObject& dataUnit)
{
    const QJsonValue typeVal = dataUnit.value(QLatin1String(Packet::Type::TYPE));
    if (typeVal.isNull() || !typeVal.isString()) {
        return;
    }
    if (typeVal.toString().compare(QLatin1String(Packet::Type::LOGIN), Qt::CaseInsensitive) == 0) {
        if (logged) {
            return;
        }
        const QJsonValue resultVal = dataUnit.value(QLatin1String(Packet::Data::SUCCESS));
        if (resultVal.isNull() || !resultVal.isBool()) {
            return;
        }
        const bool loginSuccess = resultVal.toBool();
        if (loginSuccess) {
            emit loggedIn();
            return;
        }
        const QJsonValue reasonVal = dataUnit.value(QLatin1String(Packet::Data::REASON));
        emit loginError(reasonVal.toString());
    } else if (typeVal.toString().compare(QLatin1String(Packet::Type::MESSAGE), Qt::CaseInsensitive) == 0) {
        const QJsonValue textVal = dataUnit.value(QLatin1String(Packet::Data::TEXT));
        if (textVal.isNull() || !textVal.isString()) {
            return;
        }
        const QJsonValue senderVal = dataUnit.value(QLatin1String(Packet::Data::SENDER));
        if (senderVal.isNull() || !senderVal.isString()) {
            return;
        }
        emit messageReceived(senderVal.toString(), textVal.toString());
    } else if (typeVal.toString().compare(QLatin1String(Packet::Type::USER_JOINED), Qt::CaseInsensitive) == 0) {
        const QJsonValue usernameVal = dataUnit.value(QLatin1String(Packet::Data::USERNAME));
        if (usernameVal.isNull() || !usernameVal.isString()) {
            return;
        }
        emit userJoined(usernameVal.toString());
    } else if (typeVal.toString().compare(QLatin1String(Packet::Type::USER_LEFT), Qt::CaseInsensitive) == 0) {
        const QJsonValue usernameVal = dataUnit.value(QLatin1String(Packet::Data::USERNAME));
        if (usernameVal.isNull() || !usernameVal.isString()) {
            return;
        }
        emit userLeft(usernameVal.toString());
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
                    jsonReceived(jsonDoc.object());
                }
            }
        } else {
            break;
        }
    }
}

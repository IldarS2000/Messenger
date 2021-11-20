#include <QTcpSocket>
#include <QDataStream>
#include <QJsonParseError>
#include <QJsonObject>
#include "chatclient.h"
#include "constants.h"

ChatClient::ChatClient(QObject* parent) : QObject(parent), clientSocket(new QTcpSocket(this)), logged(false)
{
    connect(clientSocket, &QTcpSocket::connected, this, &ChatClient::connected);
    connect(clientSocket, &QTcpSocket::disconnected, this, &ChatClient::disconnected);
    connect(clientSocket, &QTcpSocket::disconnected, this, [this]() -> void { logged = false; });

    connect(clientSocket, &QTcpSocket::readyRead, this, &ChatClient::onReadyRead);
    connect(clientSocket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this,
            &ChatClient::error);
}

ChatClient::~ChatClient()
{
    delete clientSocket;
}

void ChatClient::connectToServer(const QHostAddress& address, const quint16 port)
{
    clientSocket->connectToHost(address, port);
}

void ChatClient::login(const QString& userName)
{
    if (clientSocket->state() == QAbstractSocket::ConnectedState) {
        QDataStream clientStream(clientSocket);
        clientStream.setVersion(serializerVersion);

        QJsonObject dataUnit;
        dataUnit["type"] = "login";
        dataUnit["username"] = userName;
        clientStream << QJsonDocument(dataUnit).toJson(QJsonDocument::Compact);
    }
}

void ChatClient::sendMessage(const QString& message)
{
    if (message.isEmpty()) {
        return;
    }
    QDataStream clientStream(clientSocket);
    clientStream.setVersion(serializerVersion);

    QJsonObject dataUnit;
    dataUnit["type"] = "message";
    dataUnit["text"] = message;
    clientStream << QJsonDocument(dataUnit).toJson();
}

void ChatClient::disconnectFromHost()
{
    clientSocket->disconnectFromHost();
}

void ChatClient::jsonReceived(const QJsonObject& dataUnit)
{
    const QJsonValue typeVal = dataUnit.value(QLatin1String("type"));
    if (typeVal.isNull() || !typeVal.isString()) {
        return;
    }
    if (typeVal.toString().compare(QLatin1String("login"), Qt::CaseInsensitive) == 0) {
        if (logged) {
            return;
        }
        const QJsonValue resultVal = dataUnit.value(QLatin1String("success"));
        if (resultVal.isNull() || !resultVal.isBool()) {
            return;
        }
        const bool loginSuccess = resultVal.toBool();
        if (loginSuccess) {
            emit loggedIn();
            return;
        }
        const QJsonValue reasonVal = dataUnit.value(QLatin1String("reason"));
        emit loginError(reasonVal.toString());
    } else if (typeVal.toString().compare(QLatin1String("message"), Qt::CaseInsensitive) == 0) {
        const QJsonValue textVal = dataUnit.value(QLatin1String("text"));
        if (textVal.isNull() || !textVal.isString()) {
            return;
        }
        const QJsonValue senderVal = dataUnit.value(QLatin1String("sender"));
        if (senderVal.isNull() || !senderVal.isString()) {
            return;
        }
        emit messageReceived(senderVal.toString(), textVal.toString());
    } else if (typeVal.toString().compare(QLatin1String("newuser"), Qt::CaseInsensitive) == 0) {
        const QJsonValue usernameVal = dataUnit.value(QLatin1String("username"));
        if (usernameVal.isNull() || !usernameVal.isString()) {
            return;
        }
        emit userJoined(usernameVal.toString());
    } else if (typeVal.toString().compare(QLatin1String("userdisconnected"), Qt::CaseInsensitive) == 0) {
        const QJsonValue usernameVal = dataUnit.value(QLatin1String("username"));
        if (usernameVal.isNull() || !usernameVal.isString()) {
            return;
        }
        emit userLeft(usernameVal.toString());
    }
}

void ChatClient::onReadyRead()
{
    QByteArray jsonData;
    QDataStream socketStream(clientSocket);
    socketStream.setVersion(serializerVersion);

    while (true) {
        socketStream.startTransaction();
        socketStream >> jsonData;
        if (socketStream.commitTransaction()) {
            QJsonParseError parseError = {0};
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

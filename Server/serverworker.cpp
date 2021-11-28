#include <QDataStream>
#include <QJsonDocument>
#include <QJsonObject>
#include "serverworker.h"
#include "constants.h"

ServerWorker::ServerWorker(QObject* parent) : QObject(parent), serverSocket(new QTcpSocket(this))
{
    connect(serverSocket, &QTcpSocket::readyRead, this, &ServerWorker::receiveJson);
    connect(serverSocket, &QTcpSocket::disconnected, this, &ServerWorker::disconnectedFromClient);
    connect(serverSocket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this,
            &ServerWorker::error);
}

ServerWorker::~ServerWorker()
{
    delete serverSocket;
}

bool ServerWorker::setSocketDescriptor(const qintptr socketDescriptor)
{
    return serverSocket->setSocketDescriptor(socketDescriptor);
}

void ServerWorker::sendJson(const QJsonObject& json)
{
    const QByteArray jsonData = QJsonDocument(json).toJson();
    qInfo() << qPrintable(QString("sending JSON to ") + getUserName() + QString("\n") + QString::fromUtf8(jsonData));
    QDataStream socketStream(serverSocket);
    socketStream.setVersion(serializerVersion);
    socketStream << jsonData;
}

void ServerWorker::disconnectFromClient()
{
    serverSocket->disconnectFromHost();
}

QString ServerWorker::getUserName() const
{
    userNameLock.lockForRead();
    const QString result = userName;
    userNameLock.unlock();
    return result;
}

void ServerWorker::setUserName(const QString& name)
{
    userNameLock.lockForWrite();
    userName = name;
    userNameLock.unlock();
}

void ServerWorker::receiveJson()
{
    QByteArray jsonData;
    QDataStream socketStream(serverSocket);
    socketStream.setVersion(serializerVersion);
    while (true) {
        socketStream.startTransaction();
        socketStream >> jsonData;
        if (socketStream.commitTransaction()) {
            QJsonParseError parseError  = {0};
            const QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData, &parseError);
            if (parseError.error == QJsonParseError::NoError) {
                if (jsonDoc.isObject()) {
                    emit jsonReceived(jsonDoc.object());
                } else {
                    qInfo() << qPrintable(QString("invalid message: ") + QString::fromUtf8(jsonData));
                }
            } else {
                qInfo() << qPrintable(QString("invalid message: ") + QString::fromUtf8(jsonData));
            }
        } else {
            break;
        }
    }
}

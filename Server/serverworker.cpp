#include <QDataStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QSslKey>
#include "serverworker.h"
#include "constants.h"

ServerWorker::ServerWorker(QObject* parent) : QObject(parent), serverSocket(new QSslSocket(this))
{
#ifdef SSL_ENABLE
    serverSocket->setProtocol(QSsl::SslV3);
    QByteArray certificate;
    QFile fileCertificate("ssl/ssl.cert");
    if (fileCertificate.open(QIODevice::ReadOnly)) {
        certificate = fileCertificate.readAll();
        fileCertificate.close();
    } else {
        qWarning() << fileCertificate.errorString();
    }
    QSslCertificate sslCertificate(certificate);
    serverSocket->setLocalCertificate(sslCertificate);

    QByteArray privateKey;
    QFile fileKey("ssl/ssl.key");
    if (fileKey.open(QIODevice::ReadOnly)) {
        privateKey = fileKey.readAll();
        fileKey.close();
    } else {
        qWarning() << fileKey.errorString();
    }
    QSslKey sslKey(privateKey, QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey, "localhost");
    serverSocket->setPrivateKey(sslKey);
#endif

    connect(serverSocket, &QSslSocket::readyRead, this, &ServerWorker::onReadyRead);
    connect(serverSocket, &QSslSocket::disconnected, this, &ServerWorker::disconnectedFromClient);
    connect(serverSocket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this,
            &ServerWorker::error);
}

ServerWorker::~ServerWorker()
{
    delete serverSocket;
}

bool ServerWorker::setSocketDescriptor(const qintptr socketDescriptor)
{
    const bool ret = serverSocket->setSocketDescriptor(socketDescriptor);
#ifdef SSL_ENABLE
    serverSocket->startServerEncryption();
#endif
    return ret;
}

void ServerWorker::sendPacket(const QJsonObject& packet)
{
    const QByteArray jsonData = QJsonDocument(packet).toJson();
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
    QString result = userName;
    userNameLock.unlock();
    return result;
}

void ServerWorker::setUserName(const QString& name)
{
    userNameLock.lockForWrite();
    userName = name;
    userNameLock.unlock();
}

void ServerWorker::onReadyRead()
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
                    emit packetReceived(jsonDoc.object());
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

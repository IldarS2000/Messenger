#include <QThread>
#include <functional>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include "servercore.h"
#include "constants.h"

ServerCore::ServerCore(QObject* parent) : QTcpServer(parent), idealThreadCount(qMax(QThread::idealThreadCount(), 1))
{
    availableThreads.reserve(idealThreadCount);
    threadsLoad.reserve(idealThreadCount);
}

ServerCore::~ServerCore()
{
    for (QThread* singleThread : availableThreads) {
        singleThread->quit();
        singleThread->wait();
    }
}

void ServerCore::incomingConnection(const qintptr socketDescriptor)
{
    auto* worker = new ServerWorker;
    if (!worker->setSocketDescriptor(socketDescriptor)) {
        worker->deleteLater();
        return;
    }
    int threadIdx = availableThreads.size();
    if (threadIdx < idealThreadCount) {
        availableThreads.append(new QThread(this));
        threadsLoad.append(1);
        availableThreads.last()->start();
    } else {
        threadIdx = std::distance(threadsLoad.cbegin(), std::min_element(threadsLoad.cbegin(), threadsLoad.cend()));
        ++threadsLoad[threadIdx];
    }
    worker->moveToThread(availableThreads.at(threadIdx));
    connect(availableThreads.at(threadIdx), &QThread::finished, worker, &QObject::deleteLater);
    connect(worker, &ServerWorker::disconnectedFromClient, this,
            [this, worker, threadIdx] { userDisconnected(worker, threadIdx); });
    connect(worker, &ServerWorker::error, this, [this, worker] { userError(worker); });
    connect(worker, &ServerWorker::jsonReceived, this, [this, worker](auto&& placeholder) {
        jsonReceived(worker, std::forward<decltype(placeholder)>(placeholder));
    });
    connect(this, &ServerCore::stopAllClients, worker, &ServerWorker::disconnectFromClient);
    clients.append(worker);
    emit logMessage("New client Connected");
}

void ServerCore::sendJson(ServerWorker* const destination, const QJsonObject& message)
{
    Q_ASSERT(destination);
    QTimer::singleShot(0, destination, [destination, message] { destination->sendJson(message); });
}

void ServerCore::broadcast(const QJsonObject& message, ServerWorker* const exclude)
{
    for (ServerWorker* worker : clients) {
        Q_ASSERT(worker);
        if (worker != exclude) {
            sendJson(worker, message);
        }
    }
}

void ServerCore::jsonReceived(ServerWorker* const sender, const QJsonObject& json)
{
    Q_ASSERT(sender);
    emit logMessage(QLatin1String("JSON received ") + QString::fromUtf8(QJsonDocument(json).toJson()));
    const QString& userName = sender->getUserName();
    if (userName.isEmpty()) {
        return jsonFromLoggedOut(sender, json);
    }
    jsonFromLoggedIn(sender, json);
}

void ServerCore::userDisconnected(ServerWorker* const sender, const int threadIdx)
{
    --threadsLoad[threadIdx];
    clients.removeAll(sender);
    const QString& userName = sender->getUserName();
    if (!userName.isEmpty()) {
        QJsonObject dataUnit;
        dataUnit[Packet::Type::TYPE]     = Packet::Type::USER_LEFT;
        dataUnit[Packet::Data::USERNAME] = userName;
        broadcast(dataUnit, nullptr);
        emit logMessage(userName + QLatin1String(" disconnected"));
    }
    sender->deleteLater();
}

void ServerCore::userError(ServerWorker* const sender)
{
    emit logMessage(QLatin1String("Error from ") + sender->getUserName());
}

void ServerCore::stopServer()
{
    emit stopAllClients();
    close();
}

void ServerCore::jsonFromLoggedOut(ServerWorker* const sender, const QJsonObject& dataUnit)
{
    Q_ASSERT(sender);
    const QJsonValue typeVal = dataUnit.value(QLatin1String(Packet::Type::TYPE));
    if (typeVal.isNull() || !typeVal.isString()) {
        return;
    }
    if (typeVal.toString().compare(QLatin1String(Packet::Type::LOGIN), Qt::CaseInsensitive) != 0) {
        return;
    }
    const QJsonValue usernameVal = dataUnit.value(QLatin1String(Packet::Data::USERNAME));
    if (usernameVal.isNull() || !usernameVal.isString()) {
        return;
    }
    const QString newUserName = usernameVal.toString().simplified();
    if (newUserName.isEmpty()) {
        return;
    }
    for (ServerWorker* worker : qAsConst(clients)) {
        if (worker == sender) {
            continue;
        }
        if (worker->getUserName().compare(newUserName, Qt::CaseInsensitive) == 0) {
            QJsonObject message;
            message[Packet::Type::TYPE]    = Packet::Type::LOGIN;
            message[Packet::Data::SUCCESS] = false;
            message[Packet::Data::REASON]  = "duplicate username";
            sendJson(sender, message);
            return;
        }
    }
    sender->setUserName(newUserName);
    QJsonObject successMessage;
    successMessage[Packet::Type::TYPE]    = Packet::Type::LOGIN;
    successMessage[Packet::Data::SUCCESS] = true;
    sendJson(sender, successMessage);
    QJsonObject connectedMessage;
    connectedMessage[Packet::Type::TYPE]     = Packet::Type::USER_JOINED;
    connectedMessage[Packet::Data::USERNAME] = newUserName;
    broadcast(connectedMessage, sender);
}

void ServerCore::jsonFromLoggedIn(ServerWorker* const sender, const QJsonObject& docObj)
{
    Q_ASSERT(sender);
    const QJsonValue typeVal = docObj.value(QLatin1String(Packet::Type::TYPE));
    if (typeVal.isNull() || !typeVal.isString()) {
        return;
    }
    if (typeVal.toString().compare(QLatin1String(Packet::Type::MESSAGE), Qt::CaseInsensitive) != 0) {
        return;
    }
    const QJsonValue textVal = docObj.value(QLatin1String(Packet::Data::TEXT));
    if (textVal.isNull() || !textVal.isString()) {
        return;
    }
    const QString text = textVal.toString().trimmed();
    if (text.isEmpty()) {
        return;
    }
    QJsonObject message;
    message[Packet::Type::TYPE]   = Packet::Type::MESSAGE;
    message[Packet::Data::TEXT]   = text;
    message[Packet::Data::SENDER] = sender->getUserName();
    broadcast(message, sender);
}

#include <QThread>
#include <functional>
#include <QJsonDocument>
#include <QJsonObject>
#include "chatserver.h"

ChatServer::ChatServer(QObject* parent) : QTcpServer(parent), idealThreadCount(qMax(QThread::idealThreadCount(), 1))
{
    availableThreads.reserve(idealThreadCount);
    threadsLoad.reserve(idealThreadCount);
}

ChatServer::~ChatServer()
{
    for (QThread* singleThread : availableThreads) {
        singleThread->quit();
        singleThread->wait();
    }
}

void ChatServer::incomingConnection(const qintptr socketDescriptor)
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
    connect(this, &ChatServer::stopAllClients, worker, &ServerWorker::disconnectFromClient);
    clients.append(worker);
    emit logMessage("New client Connected");
}

void ChatServer::sendJson(ServerWorker* destination, const QJsonObject& message)
{
    Q_ASSERT(destination);
    destination->sendJson(message);
}

void ChatServer::broadcast(const QJsonObject& message, ServerWorker* const exclude)
{
    for (ServerWorker* worker : clients) {
        Q_ASSERT(worker);
        if (worker != exclude) {
            sendJson(worker, message);
        }
    }
}

void ChatServer::jsonReceived(ServerWorker* const sender, const QJsonObject& json)
{
    Q_ASSERT(sender);
    emit logMessage(QLatin1String("JSON received ") + QString::fromUtf8(QJsonDocument(json).toJson()));
    const QString& userName = sender->getUserName();
    if (userName.isEmpty()) {
        return jsonFromLoggedOut(sender, json);
    }
    jsonFromLoggedIn(sender, json);
}

void ChatServer::userDisconnected(ServerWorker* const sender, const int threadIdx)
{
    --threadsLoad[threadIdx];
    clients.removeAll(sender);
    const QString& userName = sender->getUserName();
    if (!userName.isEmpty()) {
        QJsonObject dataUnit;
        dataUnit["type"]     = "userdisconnected";
        dataUnit["username"] = userName;
        broadcast(dataUnit, nullptr);
        emit logMessage(userName + QLatin1String(" disconnected"));
    }
    sender->deleteLater();
}

void ChatServer::userError(ServerWorker* const sender)
{
    emit logMessage(QLatin1String("Error from ") + sender->getUserName());
}

void ChatServer::stopServer()
{
    emit stopAllClients();
    close();
}

void ChatServer::jsonFromLoggedOut(ServerWorker* const sender, const QJsonObject& dataUnit)
{
    Q_ASSERT(sender);
    const QJsonValue typeVal = dataUnit.value(QLatin1String("type"));
    if (typeVal.isNull() || !typeVal.isString()) {
        return;
    }
    if (typeVal.toString().compare(QLatin1String("login"), Qt::CaseInsensitive) != 0) {
        return;
    }
    const QJsonValue usernameVal = dataUnit.value(QLatin1String("username"));
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
            message["type"]    = "login";
            message["success"] = false;
            message["reason"]  = "duplicate username";
            sendJson(sender, message);
            return;
        }
    }
    sender->setUserName(newUserName);
    QJsonObject successMessage;
    successMessage["type"]    = "login";
    successMessage["success"] = true;
    sendJson(sender, successMessage);
    QJsonObject connectedMessage;
    connectedMessage["type"]     = "newuser";
    connectedMessage["username"] = newUserName;
    broadcast(connectedMessage, sender);
}

void ChatServer::jsonFromLoggedIn(ServerWorker* const sender, const QJsonObject& docObj)
{
    Q_ASSERT(sender);
    const QJsonValue typeVal = docObj.value(QLatin1String("type"));
    if (typeVal.isNull() || !typeVal.isString()) {
        return;
    }
    if (typeVal.toString().compare(QLatin1String("message"), Qt::CaseInsensitive) != 0) {
        return;
    }
    const QJsonValue textVal = docObj.value(QLatin1String("text"));
    if (textVal.isNull() || !textVal.isString()) {
        return;
    }
    const QString text = textVal.toString().trimmed();
    if (text.isEmpty()) {
        return;
    }
    QJsonObject message;
    message["type"]   = "message";
    message["text"]   = text;
    message["sender"] = sender->getUserName();
    broadcast(message, sender);
}

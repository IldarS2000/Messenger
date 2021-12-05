#include <QThread>
#include <functional>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include "servercore.h"
#include "constants.h"

ServerCore::ServerCore(QObject* parent) : QTcpServer(parent), idealThreadCount(qMax(QThread::idealThreadCount(), 1))
{
    threads.reserve(idealThreadCount);
    threadLoadFactor.reserve(idealThreadCount);
}

ServerCore::~ServerCore()
{
    for (QThread* singleThread : threads) {
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

    int threadIdx = threads.size();
    if (threadIdx < idealThreadCount) {
        threads.append(new QThread(this));
        threadLoadFactor.append(1);
        threads.last()->start();
    } else {
        threadIdx = static_cast<int>(std::distance(threadLoadFactor.begin(),
                                                   std::min_element(threadLoadFactor.begin(), threadLoadFactor.end())));
        ++threadLoadFactor[threadIdx];
    }

    worker->moveToThread(threads.at(threadIdx));
    connect(threads.at(threadIdx), &QThread::finished, worker, &QObject::deleteLater);
    connect(worker, &ServerWorker::disconnectedFromClient, this,
            [this, worker, threadIdx] { userDisconnected(worker, threadIdx); });

    connect(worker, &ServerWorker::error, this, [worker] { userError(worker); });
    connect(worker, &ServerWorker::packetReceived, this, [this, worker](auto&& placeholder) {
        packetReceived(worker, std::forward<decltype(placeholder)>(placeholder));
    });
    connect(this, &ServerCore::stopAllClients, worker, &ServerWorker::disconnectFromClient);
    clients.append(worker);
    qInfo() << "new client connected";
}

void ServerCore::sendPacket(ServerWorker* const destination, const QJsonObject& packet)
{
    Q_ASSERT(destination);
    QTimer::singleShot(0, destination, [destination, packet] { destination->sendPacket(packet); });
}

void ServerCore::unicast(const QJsonObject& packet, ServerWorker* const receiver)
{
    auto worker = std::find(clients.begin(), clients.end(), receiver);
    if (worker != clients.end()) {
        Q_ASSERT(*worker);
        sendPacket(*worker, packet);
    }
}

void ServerCore::broadcast(const QJsonObject& packet, ServerWorker* const exclude)
{
    for (ServerWorker* worker : clients) {
        Q_ASSERT(worker);
        if (worker != exclude) {
            sendPacket(worker, packet);
        }
    }
}

void ServerCore::packetReceived(ServerWorker* sender, const QJsonObject& packet)
{
    Q_ASSERT(sender);
    qInfo() << qPrintable("JSON received\n" + QString::fromUtf8(QJsonDocument(packet).toJson(QJsonDocument::Indented)));

    const QString& userName = sender->getUserName();
    if (userName.isEmpty()) {
        return packetFromLoggedOut(sender, packet);
    }
    packetFromLoggedIn(sender, packet);
}

void ServerCore::userDisconnected(ServerWorker* const sender, const int threadIdx)
{
    --threadLoadFactor[threadIdx];
    clients.removeAll(sender);
    const QString& userName = sender->getUserName();
    if (!userName.isEmpty()) {
        QJsonObject packet;
        packet[Packet::Type::TYPE]     = Packet::Type::USER_LEFT;
        packet[Packet::Data::USERNAME] = userName;
        broadcast(packet, nullptr);
        qInfo() << qPrintable(userName + QString(" disconnected"));
    }
    sender->deleteLater();
}

void ServerCore::userError(ServerWorker* const sender)
{
    qWarning() << qPrintable(QString("error from <") + sender->getUserName() + QString(">"));
}

void ServerCore::stopServer()
{
    emit stopAllClients();
    close();
}

bool ServerCore::isEqualPacketType(const QJsonValue& jsonType, const char* const strType)
{
    return jsonType.toString().compare(QLatin1String(strType), Qt::CaseInsensitive) == 0;
}

QJsonArray ServerCore::getUsernames(ServerWorker* const exclude) const
{
    QJsonArray usernames;
    for (ServerWorker* worker : clients) {
        Q_ASSERT(worker);
        if (worker != exclude) {
            usernames.push_back(worker->getUserName());
        }
    }
    return usernames;
}

void ServerCore::packetFromLoggedOut(ServerWorker* const sender, const QJsonObject& packet)
{
    Q_ASSERT(sender);
    const QJsonValue typeVal = packet.value(QLatin1String(Packet::Type::TYPE));
    if (typeVal.isNull() || !typeVal.isString()) {
        return;
    }
    if (!isEqualPacketType(typeVal, Packet::Type::LOGIN)) {
        return;
    }

    const QJsonValue usernameVal = packet.value(QLatin1String(Packet::Data::USERNAME));
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
            QJsonObject errorPacket;
            errorPacket[Packet::Type::TYPE]    = Packet::Type::LOGIN;
            errorPacket[Packet::Data::SUCCESS] = false;
            errorPacket[Packet::Data::REASON]  = "duplicate username";
            sendPacket(sender, errorPacket);
            return;
        }
    }

    sender->setUserName(newUserName);
    QJsonObject successPacket;
    successPacket[Packet::Type::TYPE]    = Packet::Type::LOGIN;
    successPacket[Packet::Data::SUCCESS] = true;
    sendPacket(sender, successPacket);

    QJsonObject unicastPacket;
    unicastPacket[Packet::Type::TYPE]      = Packet::Type::INFORM_JOINER;
    unicastPacket[Packet::Data::USERNAMES] = getUsernames(sender);
    unicast(unicastPacket, sender);

    QJsonObject connectedBroadcastPacket;
    connectedBroadcastPacket[Packet::Type::TYPE]     = Packet::Type::USER_JOINED;
    connectedBroadcastPacket[Packet::Data::USERNAME] = newUserName;
    broadcast(connectedBroadcastPacket, sender);
}

void ServerCore::packetFromLoggedIn(ServerWorker* const sender, const QJsonObject& packet)
{
    Q_ASSERT(sender);
    const QJsonValue typeVal = packet.value(QLatin1String(Packet::Type::TYPE));
    if (typeVal.isNull() || !typeVal.isString()) {
        return;
    }
    if (!isEqualPacketType(typeVal, Packet::Type::MESSAGE)) {
        return;
    }

    const QJsonValue textVal = packet.value(QLatin1String(Packet::Data::TEXT));
    if (textVal.isNull() || !textVal.isString()) {
        return;
    }
    const QString text = textVal.toString().trimmed();
    if (text.isEmpty()) {
        return;
    }

    QJsonObject broadcastPacket;
    broadcastPacket[Packet::Type::TYPE]   = Packet::Type::MESSAGE;
    broadcastPacket[Packet::Data::TEXT]   = text;
    broadcastPacket[Packet::Data::SENDER] = sender->getUserName();
    broadcast(broadcastPacket, sender);
}

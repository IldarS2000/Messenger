#include <QThread>
#include <functional>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include "servercore.h"
#include "db.h"
#include "constants.h"
#include "message.h"

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
    connect(worker, &ServerWorker::disconnectedFromClientSig, this,
            [this, worker, threadIdx] { userDisconnected(worker, threadIdx); });

    connect(worker, &ServerWorker::errorSig, this, [worker] { userError(worker); });
    connect(worker, &ServerWorker::packetReceivedSig, this, [this, worker](auto&& placeholder) {
        packetReceived(worker, std::forward<decltype(placeholder)>(placeholder));
    });
    connect(this, &ServerCore::stopAllClientsSig, worker, &ServerWorker::disconnectFromClient);
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

void ServerCore::broadcast(const QString& group, const QJsonObject& packet, const ServerWorker* const exclude)
{
    for (ServerWorker* const worker : clients) {
        Q_ASSERT(worker);
        if (worker != exclude) {
            if (worker->getGroupName() == group) {
                sendPacket(worker, packet);
            }
        }
    }
}

bool ServerCore::isUserLoggedIn(const QString& username)
{
    return std::any_of(clients.begin(), clients.end(),
                       [&username](ServerWorker* worker) { return worker->getUserName() == username; });
}

void ServerCore::packetReceived(ServerWorker* sender, const QJsonObject& packet)
{
    Q_ASSERT(sender);
    qInfo() << qPrintable("JSON received\n" + QString::fromUtf8(QJsonDocument(packet).toJson(QJsonDocument::Indented)));

    const QString& userName = sender->getUserName();
    if (userName.isEmpty()) {
        packetFromLoggedOut(sender, packet);
        return;
    }

    const QString& groupName = sender->getGroupName();
    if (groupName.isEmpty()) {
        packetFromLoggedIn(sender, packet);
        return;
    }

    packetFromConnectedToGroup(sender, packet);
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
        broadcast(sender->getGroupName(), packet, nullptr);
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
    emit stopAllClientsSig();
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
            if (worker->getGroupName() == exclude->getGroupName()) {
                QString username = worker->getUserName();
                if (!username.isEmpty()) {
                    usernames.push_back(qMove(username));
                }
            }
        }
    }
    return usernames;
}

QJsonArray ServerCore::getMessages(const QString& groupName)
{
    QList<Message> dbMessages = db::fetchMessages(groupName);
    QJsonArray messages;
    for (const auto& message : dbMessages) {
        QJsonObject leafObject;
        leafObject[Packet::Data::SENDER] = message.getSender();
        leafObject[Packet::Data::TEXT]   = message.getMessage();
        leafObject[Packet::Data::TIME]   = message.getTime();
        messages.push_back(leafObject);
    }
    return messages;
}

void ServerCore::packetFromLoggedOut(ServerWorker* const sender, const QJsonObject& packet)
{
    Q_ASSERT(sender);
    // check packet type
    const QJsonValue typeVal = packet.value(QLatin1String(Packet::Type::TYPE));
    if (typeVal.isNull() || !typeVal.isString()) {
        return;
    }

    if (isEqualPacketType(typeVal, Packet::Type::REGISTER)) {
        registerUser(sender, packet);
    } else if (isEqualPacketType(typeVal, Packet::Type::LOGIN)) {
        loginUser(sender, packet);
    }
}

void ServerCore::registerUser(ServerWorker* const sender, const QJsonObject& packet)
{
    // parse username
    const QJsonValue usernameVal = packet.value(QLatin1String(Packet::Data::USERNAME));
    if (usernameVal.isNull() || !usernameVal.isString()) {
        return;
    }
    const QString userName = usernameVal.toString().simplified();
    if (userName.isEmpty()) {
        return;
    }

    if (db::isUserExist(userName)) {
        QJsonObject errorPacket;
        errorPacket[Packet::Type::TYPE]    = Packet::Type::REGISTER;
        errorPacket[Packet::Data::SUCCESS] = false;
        errorPacket[Packet::Data::REASON]  = "user with such name already exist";
        sendPacket(sender, errorPacket);
        return;
    }

    // parse password
    QJsonValue passwordVal = packet.value(QLatin1String(Packet::Data::PASSWORD));
    if (passwordVal.isNull() || !passwordVal.isString()) {
        return;
    }
    QString password = passwordVal.toString().simplified();
    if (password.isEmpty()) {
        return;
    }
    db::addUser(userName, password);
    // for security reason clear sensitive info
    passwordVal = QJsonValue();
    password.clear();
    //

    // register success
    QJsonObject successPacket;
    successPacket[Packet::Type::TYPE]    = Packet::Type::REGISTER;
    successPacket[Packet::Data::SUCCESS] = true;
    sendPacket(sender, successPacket);
}

void ServerCore::loginUser(ServerWorker* const sender, const QJsonObject& packet)
{
    // parse username
    const QJsonValue usernameVal = packet.value(QLatin1String(Packet::Data::USERNAME));
    if (usernameVal.isNull() || !usernameVal.isString()) {
        return;
    }
    const QString userName = usernameVal.toString().simplified();
    if (userName.isEmpty()) {
        return;
    }

    // check user
    if (!db::isUserExist(userName)) {
        QJsonObject errorPacket;
        errorPacket[Packet::Type::TYPE]    = Packet::Type::LOGIN;
        errorPacket[Packet::Data::SUCCESS] = false;
        errorPacket[Packet::Data::REASON]  = "user with such name does not exist";
        sendPacket(sender, errorPacket);
        return;
    }

    // parse password
    QJsonValue passwordVal = packet.value(QLatin1String(Packet::Data::PASSWORD));
    if (passwordVal.isNull() || !passwordVal.isString()) {
        return;
    }
    QString password = passwordVal.toString().simplified();
    if (password.isEmpty()) {
        return;
    }

    // check password
    if (password != db::fetchUserPassword(userName)) {
        QJsonObject errorPacket;
        errorPacket[Packet::Type::TYPE]    = Packet::Type::LOGIN;
        errorPacket[Packet::Data::SUCCESS] = false;
        errorPacket[Packet::Data::REASON]  = "invalid password";
        sendPacket(sender, errorPacket);
        return;
    }
    // for security reason clear sensitive info
    passwordVal = QJsonValue();
    password.clear();
    //

    if (isUserLoggedIn(userName)) {
        QJsonObject errorPacket;
        errorPacket[Packet::Type::TYPE]    = Packet::Type::LOGIN;
        errorPacket[Packet::Data::SUCCESS] = false;
        errorPacket[Packet::Data::REASON]  = "user with such name already logged in";
        sendPacket(sender, errorPacket);
        return;
    }

    // login success
    sender->setUserName(userName);
    QJsonObject successPacket;
    successPacket[Packet::Type::TYPE]    = Packet::Type::LOGIN;
    successPacket[Packet::Data::SUCCESS] = true;
    sendPacket(sender, successPacket);
}

void ServerCore::packetFromLoggedIn(ServerWorker* sender, const QJsonObject& packet)
{
    const QJsonValue typeVal = packet.value(QLatin1String(Packet::Type::TYPE));
    if (typeVal.isNull() || !typeVal.isString()) {
        return;
    }
    if (!isEqualPacketType(typeVal, Packet::Type::CONNECT_GROUP)) {
        return;
    }

    // parse username
    const QJsonValue usernameVal = packet.value(QLatin1String(Packet::Data::USERNAME));
    if (usernameVal.isNull() || !usernameVal.isString()) {
        return;
    }
    const QString userName = usernameVal.toString().simplified();
    if (userName.isEmpty()) {
        return;
    }

    // parse group name
    const QJsonValue groupNameVal = packet.value(QLatin1String(Packet::Data::GROUP_NAME));
    if (usernameVal.isNull() || !usernameVal.isString()) {
        return;
    }
    const QString groupName = groupNameVal.toString().simplified();
    if (groupName.isEmpty()) {
        return;
    }

    if (!db::isGroupExist(groupName)) {
        QJsonObject errorPacket;
        errorPacket[Packet::Type::TYPE]    = Packet::Type::CONNECT_GROUP;
        errorPacket[Packet::Data::SUCCESS] = false;
        errorPacket[Packet::Data::REASON]  = "group with such name does not exist";
        sendPacket(sender, errorPacket);
        return;
    }

    // parse password
    QJsonValue passwordVal = packet.value(QLatin1String(Packet::Data::PASSWORD));
    if (passwordVal.isNull() || !passwordVal.isString()) {
        return;
    }
    QString password = passwordVal.toString().simplified();
    if (password.isEmpty()) {
        return;
    }

    // check password
    if (password != db::fetchGroupPassword(groupName)) {
        QJsonObject errorPacket;
        errorPacket[Packet::Type::TYPE]    = Packet::Type::CONNECT_GROUP;
        errorPacket[Packet::Data::SUCCESS] = false;
        errorPacket[Packet::Data::REASON]  = "invalid password";
        sendPacket(sender, errorPacket);
        return;
    }
    // for security reason clear sensitive info
    passwordVal = QJsonValue();
    password.clear();
    //

    // connect group success
    sender->setGroupName(groupName);
    QJsonObject successPacket;
    successPacket[Packet::Type::TYPE]    = Packet::Type::CONNECT_GROUP;
    successPacket[Packet::Data::SUCCESS] = true;
    sendPacket(sender, successPacket);

    // send all messages to user
    QJsonObject unicastPacket;
    unicastPacket[Packet::Type::TYPE]      = Packet::Type::INFORM_JOINER;
    unicastPacket[Packet::Data::USERNAMES] = getUsernames(sender);
    unicastPacket[Packet::Data::MESSAGES]  = getMessages(sender->getGroupName());
    this->unicast(unicastPacket, sender);

    // user joined broadcast
    QJsonObject connectedBroadcastPacket;
    connectedBroadcastPacket[Packet::Type::TYPE]     = Packet::Type::USER_JOINED;
    connectedBroadcastPacket[Packet::Data::USERNAME] = userName;
    this->broadcast(sender->getGroupName(), connectedBroadcastPacket, sender);
}

void ServerCore::packetFromConnectedToGroup(ServerWorker* const sender, const QJsonObject& packet)
{
    Q_ASSERT(sender);
    const QJsonValue typeVal = packet.value(QLatin1String(Packet::Type::TYPE));
    if (typeVal.isNull() || !typeVal.isString()) {
        return;
    }

    if (isEqualPacketType(typeVal, Packet::Type::CONNECT_GROUP)) {
        packetFromLoggedIn(sender, packet);
    } else if (!isEqualPacketType(typeVal, Packet::Type::MESSAGE)) {
        return;
    }

    const QJsonValue groupVal = packet.value(QLatin1String(Packet::Data::GROUP_NAME));
    if (groupVal.isNull() || !groupVal.isString()) {
        return;
    }
    const QString groupName = groupVal.toString();
    if (groupName.isEmpty()) {
        return;
    }

    const QJsonValue senderVal = packet.value(QLatin1String(Packet::Data::SENDER));
    if (senderVal.isNull() || !senderVal.isString()) {
        return;
    }
    const QString senderName = senderVal.toString();
    if (senderName.isEmpty()) {
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

    const QJsonValue timeVal = packet.value(QLatin1String(Packet::Data::TIME));
    if (timeVal.isNull() || !timeVal.isString()) {
        return;
    }
    const QString time = timeVal.toString();
    if (time.isEmpty()) {
        return;
    }

    QJsonObject broadcastPacket;
    broadcastPacket[Packet::Type::TYPE]   = Packet::Type::MESSAGE;
    broadcastPacket[Packet::Data::SENDER] = sender->getUserName();
    broadcastPacket[Packet::Data::TEXT]   = text;
    broadcastPacket[Packet::Data::TIME]   = time;
    broadcast(sender->getGroupName(), broadcastPacket, sender);

    db::addMessage({groupName, senderName, text, time});
}

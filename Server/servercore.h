#ifndef SERVERCORE_H
#define SERVERCORE_H

#include <QTcpServer>
#include <QVector>
#include <QThread>
#include <QJsonObject>
#include "serverworker.h"

class ServerCore : public QTcpServer
{
    Q_OBJECT
    Q_DISABLE_COPY(ServerCore)
public:
    explicit ServerCore(QObject* parent = nullptr);
    ~ServerCore() override;

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private:
    const int idealThreadCount;
    QVector<QThread*> availableThreads;
    QVector<int> threadsLoad;
    QVector<ServerWorker*> clients;
private slots:
    void broadcast(const QJsonObject& message, ServerWorker* exclude);
    void jsonReceived(ServerWorker* sender, const QJsonObject& doc);
    void userDisconnected(ServerWorker* sender, int threadIdx);
    void userError(ServerWorker* sender);
public slots:
    void stopServer();

private:
    void jsonFromLoggedOut(ServerWorker* sender, const QJsonObject& dataUnit);
    void jsonFromLoggedIn(ServerWorker* sender, const QJsonObject& doc);
    void sendJson(ServerWorker* destination, const QJsonObject& message);
signals:
    void logMessage(const QString& msg);
    void stopAllClients();
};

#endif // SERVERCORE_H

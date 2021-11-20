#ifndef SERVERWORKER_H
#define SERVERWORKER_H

#include <QObject>
#include <QTcpSocket>
#include <QReadWriteLock>
#include <QJsonObject>

class ServerWorker : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(ServerWorker)
public:
    explicit ServerWorker(QObject* parent = nullptr);
    virtual bool setSocketDescriptor(qintptr socketDescriptor);
    QString getUserName() const;
    void setUserName(const QString& name);
    void sendJson(const QJsonObject& json);
public slots:
    void disconnectFromClient();
private slots:
    void receiveJson();
signals:
    void jsonReceived(const QJsonObject& jsonDoc);
    void disconnectedFromClient();
    void error();
    void logMessage(const QString& msg);

private:
    QTcpSocket* serverSocket;
    QString userName;
    mutable QReadWriteLock userNameLock;
};

#endif // SERVERWORKER_H

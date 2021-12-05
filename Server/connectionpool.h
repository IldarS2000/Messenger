#ifndef MESSENGER_CONNECTIONPOOL_H
#define MESSENGER_CONNECTIONPOOL_H

#include <QtSql>
#include <QQueue>
#include <QString>
#include <QMutex>
#include <QMutexLocker>

class ConnectionPool
{
    Q_DISABLE_COPY(ConnectionPool)
public:
    static void release();
    static QSqlDatabase getConnection();
    static void releaseConnection(const QSqlDatabase& connection);
    ~ConnectionPool();

private:
    ConnectionPool() = default;
    static ConnectionPool& getInstance();
    static QSqlDatabase createConnection(const QString& connectionName);

private:
    QQueue<QString> usedConnectionNames;
    QQueue<QString> unusedConnectionNames;

    static constexpr const char* const testOnBorrowSql = "SELECT 1";
    static constexpr bool testOnBorrow                 = true;
    static constexpr int maxWaitTime                   = 1000;
    static constexpr int waitInterval                  = 200;
    static constexpr int maxConnectionCount            = 5;
    static inline QMutex mutex{};
    static inline QWaitCondition waitConnection{};
    static inline ConnectionPool* instance = nullptr;
};

#endif // MESSENGER_CONNECTIONPOOL_H

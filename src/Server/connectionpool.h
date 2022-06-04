#ifndef CONNECTIONPOOL_H
#define CONNECTIONPOOL_H

#include <QtSql>
#include <QQueue>
#include <QString>
#include <QMutex>
#include <QMutexLocker>

class ConnectionPool
{
    Q_DISABLE_COPY(ConnectionPool)
public:
    static QSqlDatabase getConnection();
    static void releaseConnection(const QSqlDatabase& connection);
    static void release();
    ~ConnectionPool();

private:
    ConnectionPool() = default;
    static ConnectionPool& getInstance();
    static QSqlDatabase createConnection(const QString& connectionName);

private:
    QQueue<QString> usedConnectionNames;
    QQueue<QString> unusedConnectionNames;

    static constexpr const char* const testOnBorrowQuery = "SELECT 1";
    static constexpr bool testOnBorrow                   = true;
    static constexpr int maxWaitTime                     = 1000;
    static constexpr int waitInterval                    = 200;
    static constexpr int maxConnectionCount              = 4;
    static inline QMutex mutex{};
    static inline QWaitCondition waitConnection{};
    static inline ConnectionPool* instance = nullptr;
};

#endif // CONNECTIONPOOL_H

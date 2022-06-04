#ifndef CONNECTIONPOOL_H
#define CONNECTIONPOOL_H

#include <QtSql>
#include <QQueue>
#include <QString>
#include <QMutex>
#include <QMutexLocker>

class ConnectionPool : QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(ConnectionPool)
public:
    static QSqlDatabase getConnection();
    static void releaseConnection(const QSqlDatabase& connection);
    static void release();
    ~ConnectionPool() override;

private:
    ConnectionPool();
    static ConnectionPool& getInstance();
    static QSqlDatabase createConnection(const QString& connectionName);

private slots:
    void releaseUnusedConnections();

private:
    struct UnusedConnection {
        QString connectionName;
        bool released;
    };
    QTimer* timer;
    QQueue<QString> usedConnections;
    QQueue<UnusedConnection> unusedConnections;

    static constexpr const char* const testOnBorrowQuery = "SELECT 1";
    static constexpr bool testOnBorrow                   = true;
    static constexpr int maxWaitTime                     = 1000;
    static constexpr int waitInterval                    = 200;
    static constexpr int maxConnectionCount              = 50;
    static constexpr int releaseConnectionInterval       = 1000 * 300;
    static inline QMutex mutex{};
    static inline QWaitCondition waitConnection{};
    static inline ConnectionPool* instance = nullptr;
};

#endif // CONNECTIONPOOL_H

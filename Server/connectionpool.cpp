#include <QUuid>
#include "connectionpool.h"
#include "config.h"

ConnectionPool::~ConnectionPool()
{
    foreach (QString connectionName, usedConnectionNames) {
        QSqlDatabase::removeDatabase(connectionName);
    }
}

ConnectionPool& ConnectionPool::getInstance()
{
    if (instance == nullptr) {
        QMutexLocker locker(&mutex);
        if (instance == nullptr) {
            instance = new ConnectionPool();
        }
    }
    return *instance;
}

void ConnectionPool::release()
{
    QMutexLocker locker(&mutex);
    delete instance;
    instance = nullptr;
}

QSqlDatabase ConnectionPool::getConnection()
{
    ConnectionPool& pool = ConnectionPool::getInstance();

    QMutexLocker locker(&mutex);
    int connectionCount = pool.unusedConnectionNames.size() + pool.usedConnectionNames.size();
    for (int i = 0; i < ConnectionPool::maxWaitTime && pool.unusedConnectionNames.empty() &&
                    connectionCount == ConnectionPool::maxConnectionCount;
         i += ConnectionPool::waitInterval) {
        waitConnection.wait(&mutex, ConnectionPool::waitInterval);
        connectionCount = pool.unusedConnectionNames.size() + pool.usedConnectionNames.size();
    }

    QString connectionName;
    if (!pool.unusedConnectionNames.empty()) {
        connectionName = pool.unusedConnectionNames.dequeue();
    } else if (connectionCount < ConnectionPool::maxConnectionCount) {
        connectionName = QString("conn-%1").arg(QUuid::createUuid().toString());
    } else {
        qInfo() << "cannot create more connections";
        return {};
    }

    QSqlDatabase db = ConnectionPool::createConnection(connectionName);
    if (db.isOpen()) {
        pool.usedConnectionNames.enqueue(connectionName);
    }
    return db;
}

void ConnectionPool::releaseConnection(const QSqlDatabase& connection)
{
    ConnectionPool& pool   = ConnectionPool ::getInstance();
    QString connectionName = connection.connectionName();
    if (pool.usedConnectionNames.contains(connectionName)) {
        QMutexLocker locker(&mutex);
        pool.usedConnectionNames.removeOne(connectionName);
        pool.unusedConnectionNames.enqueue(connectionName);
        waitConnection.wakeOne();
    }
}

QSqlDatabase ConnectionPool::createConnection(const QString& connectionName)
{
    if (QSqlDatabase::contains(connectionName)) {
        QSqlDatabase db = QSqlDatabase::database(connectionName);

        if (testOnBorrow) {
            QSqlQuery query(testOnBorrowSql, db);
            if (query.lastError().type() != QSqlError::NoError && !db.open()) {
                qWarning() << "DB fail:" << db.lastError().text();
                return {};
            }
        }

        return db;
    }

    QSqlDatabase db = QSqlDatabase::addDatabase(DB_TYPE, connectionName);
    db.setHostName(DB_HOSTNAME);
    db.setDatabaseName(DB_NAME);
    db.setUserName(DB_USERNAME);
    db.setPassword(DB_PASSWORD);

    if (!db.open()) {
        qWarning() << "DB fail:" << db.lastError().text();
        return {};
    }
    qInfo() << "DB connection <" << connectionName << "> ok";

    return db;
}

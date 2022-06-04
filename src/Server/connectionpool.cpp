#include <QUuid>
#include <QTimer>
#include "connectionpool.h"
#include "config.h"

ConnectionPool::ConnectionPool()
{
    timer = new QTimer();
    timer->setInterval(releaseConnectionInterval);
    timer->setSingleShot(false);
    connect(timer, &QTimer::timeout, this, [this]() { releaseUnusedConnections(); });
    timer->start();
}

ConnectionPool::~ConnectionPool()
{
    timer->stop();
    for (const auto& connectionName : usedConnections) {
        QSqlDatabase::removeDatabase(connectionName);
    }
    delete timer;
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
    int connectionCount = pool.unusedConnections.size() + pool.usedConnections.size();
    for (int i = 0; i < ConnectionPool::maxWaitTime && pool.unusedConnections.empty() &&
                    connectionCount == ConnectionPool::maxConnectionCount;
         i += ConnectionPool::waitInterval) {
        waitConnection.wait(&mutex, ConnectionPool::waitInterval);
        connectionCount = pool.unusedConnections.size() + pool.usedConnections.size();
    }

    QString connectionName;
    if (!pool.unusedConnections.empty()) {
        connectionName = pool.unusedConnections.dequeue().connectionName;
    } else if (connectionCount < ConnectionPool::maxConnectionCount) {
        connectionName = QString(QUuid::createUuid().toString());
    } else {
        qInfo() << "cannot create more connections";
        return {};
    }

    QSqlDatabase db = ConnectionPool::createConnection(connectionName);
    if (db.isOpen()) {
        pool.usedConnections.enqueue(connectionName);
    }
    return db;
}

void ConnectionPool::releaseConnection(const QSqlDatabase& connection)
{
    ConnectionPool& pool   = ConnectionPool::getInstance();
    QString connectionName = connection.connectionName();
    if (pool.usedConnections.contains(connectionName)) {
        QMutexLocker locker(&mutex);
        pool.usedConnections.removeOne(connectionName);
        pool.unusedConnections.enqueue({connectionName, false});
        waitConnection.wakeOne();
    }
}

QSqlDatabase ConnectionPool::createConnection(const QString& connectionName)
{
    if (QSqlDatabase::contains(connectionName)) {
        QSqlDatabase db = QSqlDatabase::database(connectionName);

        if (testOnBorrow) {
            QSqlQuery query(testOnBorrowQuery, db);
            if (query.lastError().type() != QSqlError::NoError && !db.open()) {
                qWarning() << qPrintable(QString("DB fail:") + db.lastError().text());
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
        qWarning() << qPrintable(QString("DB fail:") + db.lastError().text());
        return {};
    }
    qInfo() << qPrintable(QString("DB connection ") + connectionName + QString(" ok"));

    return db;
}

void ConnectionPool::releaseUnusedConnections()
{
    QMutexLocker locker(&mutex);
    for (auto& unusedConnection : unusedConnections) {
        if (!unusedConnection.released) {
            QSqlDatabase::removeDatabase(unusedConnection.connectionName);
            qInfo() << "released connection: " << unusedConnection.connectionName;
            unusedConnection.released = true;
        }
    }
}

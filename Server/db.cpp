#include <QString>
#include <QSqlDatabase>
#include <QSqlQuery>
#include "servercore.h"
#include "connectionpool.h"
#include "db.h"

QString db::fetchGroupPassword(const QString& groupName)
{
    auto conn = ConnectionPool::getConnection();
    QSqlQuery query(conn);
    query.prepare(R"(select * from "Messenger".public.group where "name" = :name)");
    query.bindValue(":name", groupName);
    query.exec();

    QString password;
    while (query.next()) {
        password = query.value("password").toString();
    }
    ConnectionPool::releaseConnection(conn);

    return password;
}

void db::addMessage(const Message& message) {}

db::Message db::fetchMessages(const QString& groupName) {}
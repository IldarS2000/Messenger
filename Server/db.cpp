#include <QString>
#include <QSqlDatabase>
#include <QSqlQuery>
#include "servercore.h"
#include "connectionpool.h"
#include "db.h"

bool db::isUserExist(const QString& userName)
{
    auto conn = ConnectionPool::getConnection();
    QSqlQuery query(conn);
    query.prepare(R"(select id from "Messenger".public.user where "name" = :name)");
    query.bindValue(":name", userName);
    query.exec();

    int id = 0;
    while (query.next()) {
        id = query.value("id").toInt();
    }
    ConnectionPool::releaseConnection(conn);

    return id != 0;
}

QString db::fetchUserPassword(const QString& userName)
{
    auto conn = ConnectionPool::getConnection();
    QSqlQuery query(conn);
    query.prepare(R"(select password from "Messenger".public.user where "name" = :name)");
    query.bindValue(":name", userName);
    query.exec();

    QString password;
    while (query.next()) {
        password = query.value("password").toString();
    }
    ConnectionPool::releaseConnection(conn);

    return password;
}

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

void db::addMessage(const Message& message)
{
    auto conn = ConnectionPool::getConnection();
    QSqlQuery query(conn);
    query.prepare(R"(insert into message (group_id, sender_name, message, time)
values (1, :sender_name, :message, :time))");
    query.bindValue(":sender_name", message.getSender());
    query.bindValue(":message", message.getMessage());
    query.bindValue(":time", message.getTime());
    query.exec();
    ConnectionPool::releaseConnection(conn);
}

QList<Message> db::fetchMessages(const QString& groupName)
{
    auto conn = ConnectionPool::getConnection();
    QSqlQuery query(conn);
    query.prepare(R"(select m.sender_name, m.message, m.time
from message m inner join "group" g on g.id = m.group_id
where g.name = :name)");
    query.bindValue(":name", groupName);
    query.exec();

    QList<Message> messages;
    while (query.next()) {
        QString senderName = query.value("sender_name").toString();
        QString text       = query.value("message").toString();
        QString time       = query.value("time").toString();
        messages.push_back({qMove(senderName), qMove(text), qMove(time)});
    }
    ConnectionPool::releaseConnection(conn);


    return messages;
}

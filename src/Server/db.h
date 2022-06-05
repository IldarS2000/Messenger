#ifndef DB_H
#define DB_H

#include <QString>
#include "message.h"

namespace db {
    bool isUserExist(const QString& userName);
    bool isGroupExist(const QString& groupName);
    void addUser(const QString& userName, const QString& password);
    void addGroup(const QString& groupName, const QString& password);
    QString fetchUserPassword(const QString& userName);
    QString fetchGroupPassword(const QString& groupName);
    void addMessage(const Message& message);
    QList<Message> fetchMessages(const QString& groupName);
} // namespace db

#endif // DB_H

#ifndef DB_H
#define DB_H

#include "message.h"

namespace db {
    QString fetchGroupPassword(const QString& groupName);
    void addMessage(const Message& message);
    QList<Message> fetchMessages(const QString& groupName);
}; // namespace db

#endif // DB_H

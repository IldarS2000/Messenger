#ifndef MESSENGER_DB_H
#define MESSENGER_DB_H

#include "message.h"

namespace db {
    QString fetchGroupPassword(const QString& groupName);
    void addMessage(const Message& message);
    QList<Message> fetchMessages(const QString& groupName);
}; // namespace db

#endif // MESSENGER_DB_H

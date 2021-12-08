#ifndef MESSENGER_DB_H
#define MESSENGER_DB_H

namespace db {
    struct Message {
        QString message;
        QString sender;
        QString time;
    };

    QString fetchGroupPassword(const QString& groupName);
    void addMessage(const Message& message);
    Message fetchMessages(const QString& groupName);
}; // namespace db

#endif // MESSENGER_DB_H

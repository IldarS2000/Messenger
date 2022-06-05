#include "message.h"

Message::Message(const QString& groupName, const QString& sender, const QString& message, const QString& time)
    : groupName(groupName), sender(sender), message(message), time(time)
{}

const QString& Message::getGroupName() const
{
    return groupName;
}

const QString& Message::getSender() const
{
    return sender;
}

const QString& Message::getMessage() const
{
    return message;
}

const QString& Message::getTime() const
{
    return time;
}
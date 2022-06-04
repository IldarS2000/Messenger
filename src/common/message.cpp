#include "message.h"

Message::Message(const QString& sender, const QString& message, const QString& time)
    : sender(sender), message(message), time(time)
{}

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

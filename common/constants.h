#ifndef MESSENGER_CONSTANTS_H
#define MESSENGER_CONSTANTS_H

constexpr auto serializerVersion       = QDataStream::Qt_5_7;
constexpr quint16 PORT                 = 30000;
constexpr const char* const LOCAL_HOST = "127.0.0.1";

namespace DataUnit {
    constexpr const char* const TYPE      = "type";
    constexpr const char* const LOGIN     = "login";
    constexpr const char* const MESSAGE   = "message";
    constexpr const char* const NEWUSER    = "newuser";
    constexpr const char* const USERDISCONNECTED = "userdisconnected";
} // namespace DataUnit

#endif // MESSENGER_CONSTANTS_H

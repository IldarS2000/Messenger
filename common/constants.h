#ifndef MESSENGER_CONSTANTS_H
#define MESSENGER_CONSTANTS_H

constexpr auto serializerVersion       = QDataStream::Qt_5_7;
constexpr quint16 PORT                 = 30000;
constexpr const char* const LOCAL_HOST = "127.0.0.1";

namespace DataUnit {
    constexpr const char* const TYPE      = "type";
    constexpr const char* const LOGIN     = "type";
    constexpr const char* const REASON    = "type";
    constexpr const char* const MESSAGE   = "type";
    constexpr const char* const SENDER    = "type";
    constexpr const char* const USER_JOIN = "type";
    constexpr const char* const USER_LEFT = "type";
} // namespace DataUnit

#endif // MESSENGER_CONSTANTS_H

#ifndef MESSENGER_CONSTANTS_H
#define MESSENGER_CONSTANTS_H

#include "QDataStream"

constexpr auto serializerVersion = QDataStream::Qt_5_7;
constexpr quint16 PORT           = 30000;
constexpr const char* const HOST = "127.0.0.1";

namespace Packet {
    namespace Type {
        constexpr const char* const TYPE          = "type";
        constexpr const char* const LOGIN         = "login";
        constexpr const char* const USER_JOINED   = "user_joined";
        constexpr const char* const USER_LEFT     = "user_left";
        constexpr const char* const MESSAGE       = "message";
        constexpr const char* const INFORM_JOINER = "inform_joiner";
    } // namespace Type
    namespace Data {
        constexpr const char* const USERNAME  = "username";
        constexpr const char* const PASSWORD  = "password";
        constexpr const char* const TEXT      = "text";
        constexpr const char* const SENDER    = "sender";
        constexpr const char* const SUCCESS   = "success";
        constexpr const char* const REASON    = "reason";
        constexpr const char* const USERNAMES = "usernames";
    } // namespace Data
} // namespace Packet

#endif // MESSENGER_CONSTANTS_H

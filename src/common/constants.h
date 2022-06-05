#ifndef MESSENGER_CONSTANTS_H
#define MESSENGER_CONSTANTS_H

#include "QDataStream"

constexpr auto SERIALIZER_VERSION = QDataStream::Qt_5_7;
constexpr quint16 PORT            = 30000;
constexpr const char* const HOST  = "127.0.0.1";
constexpr int MAX_USER_NAME_SIZE  = 32;
constexpr int MAX_GROUP_NAME_SIZE = 32;
constexpr int MIN_PASSWORD_SIZE   = 8;
constexpr int MAX_PASSWORD_SIZE   = 32;

namespace Packet {
    namespace Type {
        constexpr const char* const TYPE          = "type";
        constexpr const char* const LOGIN         = "login";
        constexpr const char* const REGISTER      = "register";
        constexpr const char* const CONNECT_GROUP = "connect_group";
        constexpr const char* const USER_JOINED   = "user_joined";
        constexpr const char* const USER_LEFT     = "user_left";
        constexpr const char* const MESSAGE       = "message";
        constexpr const char* const INFORM_JOINER = "inform_joiner";
    } // namespace Type
    namespace Data {
        constexpr const char* const USERNAME   = "username";
        constexpr const char* const GROUP_NAME = "group_name";
        constexpr const char* const PASSWORD   = "password";
        constexpr const char* const TEXT       = "text";
        constexpr const char* const SENDER     = "sender";
        constexpr const char* const SUCCESS    = "success";
        constexpr const char* const REASON     = "reason";
        constexpr const char* const USERNAMES  = "usernames";
        constexpr const char* const MESSAGES   = "messages";
        constexpr const char* const TIME       = "time";
    } // namespace Data
} // namespace Packet

#endif // MESSENGER_CONSTANTS_H

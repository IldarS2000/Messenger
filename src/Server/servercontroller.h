#ifndef SERVER_CONTROLLER_H
#define SERVER_CONTROLLER_H

#include "servercore.h"

class ServerController
{
public:
    ServerController();
    ~ServerController();
    void startServer();

private:
    ServerCore* serverCore;
};

#endif // SERVER_CONTROLLER_H

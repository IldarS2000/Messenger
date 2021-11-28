#include "servercontroller.h"
#include "constants.h"

ServerController::ServerController() : serverCore(new ServerCore()) {}

ServerController::~ServerController()
{
    delete serverCore;
}

void ServerController::startServer()
{
    if (serverCore->isListening()) {
        serverCore->stopServer();
        qInfo() << "server stopped";
    } else {
        if (!serverCore->listen(QHostAddress::Any, PORT)) {
            qCritical() << "unable to start the server";
            return;
        }
        qInfo() << "server started";
    }
}

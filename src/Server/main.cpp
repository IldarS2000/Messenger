#include <QApplication>
#include <exception>
#include "servercontroller.h"
#include "utils.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    qInstallMessageHandler(messageHandler);

    qInfo() << "--- start server ---";
    try {
        ServerController server;
        server.startServer();
    } catch (const std::exception& e) {
        qCritical() << "critical error occurred: " << e.what();
    }

    return QApplication::exec();
}

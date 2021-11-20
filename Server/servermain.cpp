#include <QApplication>
#include "serverwindow.h"
#include "utils.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    qInstallMessageHandler(messageHandler);
    qInfo() << "--- launch server ---";

    ServerWindow server;
    server.show();

    return QApplication::exec();
}

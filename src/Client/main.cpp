#include <QApplication>
#include <exception>
#include "clientwindow.h"
#include "utils.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    qInstallMessageHandler(messageHandler);

    qInfo() << "--- start client ---";
    ClientWindow client;
    client.show();

    return QApplication::exec();
}

#include <QApplication>
#include <exception>
#include "clientwindow.h"
#include "utils.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    qInstallMessageHandler(messageHandler);

    qInfo() << "--- start client ---";
    try {
        ClientWindow client;
        client.show();
    } catch (const std::exception& e) {
        qCritical() << "critical error occurred: " << e.what();
    }

    return QApplication::exec();
}

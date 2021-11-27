#include <QApplication>
#include "clientwindow.h"
#include "utils.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
   // app.setWindowIcon(QIcon("./Client/recources/appIcon"));
    qInstallMessageHandler(messageHandler);

    qInfo() << "--- launch client ---";
    ClientWindow client;
    client.show();

    return QApplication::exec();
}

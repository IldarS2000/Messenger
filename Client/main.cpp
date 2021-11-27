#include <QApplication>
#include "clientwindow.h"
#include "utils.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    qInstallMessageHandler(messageHandler);

    qInfo() << "--- launch client ---";
    ClientWindow window;
    window.show();

    return QApplication::exec();
}

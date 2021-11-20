#include <QApplication>
#include "chatwindow.h"
#include "utils.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    qInstallMessageHandler(messageHandler);
    qInfo() << "--- launch client ---";

    ChatWindow chatWin;
    chatWin.show();

    return QApplication::exec();
}

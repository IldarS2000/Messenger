#include <QApplication>
#include "serverwindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    ServerWindow serverWin;
    serverWin.show();

    return QApplication::exec();
}

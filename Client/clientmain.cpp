#include <QApplication>
#include "chatwindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    ChatWindow chatWin;
    chatWin.show();

    return QApplication::exec();
}

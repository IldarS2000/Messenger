#ifndef SERVERWINDOW_H
#define SERVERWINDOW_H

#include <QWidget>
#include "servercore.h"

namespace Ui {
    class ServerWindow;
}

class ServerWindow : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY(ServerWindow)
public:
    explicit ServerWindow(QWidget* parent = nullptr);
    ~ServerWindow() override;

private slots:
    void toggleStartServer();
    void logMessage(const QString& msg);

private:
    Ui::ServerWindow* ui;
    ServerCore* chatServer;
};

#endif // SERVERWINDOW_H

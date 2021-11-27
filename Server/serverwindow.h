#ifndef SERVER_WINDOW_H
#define SERVER_WINDOW_H

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
    ServerCore* serverCore;
};

#endif // SERVER_WINDOW_H

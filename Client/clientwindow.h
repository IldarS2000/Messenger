#ifndef CLIENT_WINDOW_H
#define CLIENT_WINDOW_H

#include <QWidget>
#include <QAbstractSocket>
#include <QStandardItemModel>
#include "clientcore.h"

namespace Ui {
    class ClientWindow;
}

class ClientWindow : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY(ClientWindow)
public:
    explicit ClientWindow(QWidget* parent = nullptr);
    ~ClientWindow() override;

private:
    Ui::ClientWindow* ui;
    ClientCore* clientCore;
    QStandardItemModel* chatModel;
    QString lastUserName;
    static constexpr int MIN_WINDOW_WIDTH     = 750;
    static constexpr int MIN_WINDOW_HEIGHT    = 500;
    static constexpr int MAX_MESSAGE_ROW_SIZE = 50;
private slots:
    void attemptConnection();
    void connected();
    void attemptLogin(const QString& userName);
    void loggedIn();
    void loginError(const QString& reason);
    void messageReceived(const QString& sender, const QString& message);
    void sendMessage();
    void disconnected();
    void userJoined(const QString& username);
    void userLeft(const QString& username);
    void error(QAbstractSocket::SocketError socketError);

private:
    static QStringList splitString(const QString& str, int rowSize);
    static QStringList splitText(const QString& text);
    void displayMessage(const QString& message, int lastRowNumber, int alignMask);
    void userEventImpl(const QString& username, const QString& event);
};

#endif // CLIENT_WINDOW_H

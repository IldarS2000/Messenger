#ifndef CLIENT_WINDOW_H
#define CLIENT_WINDOW_H

#include <QWidget>
#include <QAbstractSocket>
#include <QStandardItemModel>
#include "login.h"
#include "register.h"
#include "clientcore.h"
#include "loadingscreen.h"
#include "message.h"

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
    Login* loginWindow;
    Register* registerWindow;
    LoadingScreen* loadingScreen;
    bool logged;
    static constexpr int minWindowWidth    = 750;
    static constexpr int minWindowHeight   = 500;
    static constexpr int maxMessageRowSize = 50;
    static constexpr int maxMessageSize    = 2048;
    static constexpr int minUserNameSize   = 1;
    static constexpr int maxUserNameSize   = 32;
    static constexpr int minPasswordSize   = 4;
    static constexpr int maxPasswordSize   = 32;
private slots:
    void attemptConnection();
    void connected();
    void attemptLogin(const QString& username, const QString& password);
    void loggedIn();
    void loginError(const QString& reason);
    void messageReceived(const Message& message);
    void sendMessage();
    void disconnected();
    void userJoined(const QString& username);
    void userLeft(const QString& username);
    void informJoiner(const QStringList& usernames, const QList<Message>& messages);
    void error(QAbstractSocket::SocketError socketError);

private:
    QPair<QString, QString> getConnectionCredentials();
    static QStringList splitString(const QString& str, int rowSize);
    static QStringList splitText(const QString& text);
    void displayMessage(const QString& message, const QString& time, int lastRowNumber, int alignMask);
    void userEventImpl(const QString& username, const QString& event);

    void signInClicked();
};

#endif // CLIENT_WINDOW_H

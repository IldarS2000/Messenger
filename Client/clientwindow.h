#ifndef CLIENT_WINDOW_H
#define CLIENT_WINDOW_H

#include <QWidget>
#include <QAbstractSocket>
#include <QStandardItemModel>
#include "clientcore.h"
#include "loadingscreen.h"

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
    LoadingScreen* loadingScreen;
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
    void messageReceived(const QString& sender, const QString& message);
    void sendMessage();
    void disconnected();
    void userJoined(const QString& username);
    void userLeft(const QString& username);
    void informJoiner(const QStringList& usernames);
    void error(QAbstractSocket::SocketError socketError);

private:
    QPair<QString, QString> getConnectionCredentials();
    static QStringList splitString(const QString& str, int rowSize);
    static QStringList splitText(const QString& text);
    void displayMessage(const QString& message, int lastRowNumber, int alignMask);
    void userEventImpl(const QString& username, const QString& event);
};

#endif // CLIENT_WINDOW_H

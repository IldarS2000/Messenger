#ifndef CLIENT_WINDOW_H
#define CLIENT_WINDOW_H

#include <QWidget>
#include <QAbstractSocket>
#include <QStandardItemModel>
#include "login.h"
#include "register.h"
#include "clientcore.h"
#include "loadingscreen.h"
#include "creategroup.h"
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
    CreateGroup* createGroupWindow;
    LoadingScreen* loadingScreen;
    bool logged;
    static constexpr int minWindowWidth    = 750;
    static constexpr int minWindowHeight   = 500;
    static constexpr int maxMessageRowSize = 50;
    static constexpr int maxMessageSize    = 2048;
private slots:
    void connected();
    void loggedIn();
    void registered();
    void connectedToGroup();
    void createdGroup();
    void loginError(const QString& reason);
    void registerError(const QString& reason);
    void connectGroupError(const QString& reason);
    void createdGroupError(const QString& reason);
    void messageReceived(const Message& message);
    void sendMessage();
    void disconnected();
    void userJoined(const QString& username);
    void userLeft(const QString& username);
    void informJoiner(const QStringList& usernames, const QList<Message>& messages);
    void error(QAbstractSocket::SocketError socketError);
    void signInClicked();
    void loginSignUpClicked();
    void registerSignUpClicked();
    void createGroupClicked();
    void createGroupWindowClicked();
    void connectGroupClicked();

private:
    static QString encryptPassword(const QString& password);
    void attemptConnection();
    void attemptLogin(const QString& username, const QString& password);
    void attemptRegister(const QString& username, const QString& password);
    void attemptConnectGroup(const QString& groupName, const QString& password);
    void attemptCreateGroup(const QString& groupName, const QString& password);
    void enableUi();
    void disableUi();

    QPair<QString, QString> getConnectionCredentials();
    static QStringList splitString(const QString& str, int rowSize);
    static QStringList splitText(const QString& text);
    void displayMessage(const QString& message, const QString& time, int lastRowNumber, int alignMask);
    void userEventImpl(const QString& username, const QString& event);
};

#endif // CLIENT_WINDOW_H

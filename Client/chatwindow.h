#ifndef CHATWINDOW_H
#define CHATWINDOW_H

#include <QWidget>
#include <QAbstractSocket>
#include <QStandardItemModel>
#include "chatclient.h"

namespace Ui {
    class ChatWindow;
}

class ChatWindow : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY(ChatWindow)
public:
    explicit ChatWindow(QWidget* parent = nullptr);
    ~ChatWindow() override;

private:
    Ui::ChatWindow* ui;
    ChatClient* chatClient;
    QStandardItemModel* chatModel;
    QString lastUserName;
private slots:
    void attemptConnection();
    void connectedToServer();
    void attemptLogin(const QString& userName);
    void loggedIn();
    void loginFailed(const QString& reason);
    void messageReceived(const QString& sender, const QString& message);
    void sendMessage();
    void disconnectedFromServer();
    void userJoined(const QString& username);
    void userLeft(const QString& username);
    void error(QAbstractSocket::SocketError socketError);

private:
    void userEventImpl(const QString& username, const QString& event);
};

#endif // CHATWINDOW_H

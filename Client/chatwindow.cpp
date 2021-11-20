#include <QStandardItemModel>
#include <QInputDialog>
#include <QMessageBox>
#include <QHostAddress>
#include "chatwindow.h"
#include "ui_window.h"
#include "constants.h"

ChatWindow::ChatWindow(QWidget* parent)
    : QWidget(parent), ui(new Ui::ChatWindow), chatClient(new ChatClient(this)), chatModel(new QStandardItemModel(this))
{
    ui->setupUi(this);
    chatModel->insertColumn(0);
    ui->chatView->setModel(chatModel);

    // handle signals from logic view of client
    connect(chatClient, &ChatClient::connected, this, &ChatWindow::connectedToServer);
    connect(chatClient, &ChatClient::loggedIn, this, &ChatWindow::loggedIn);
    connect(chatClient, &ChatClient::loginError, this, &ChatWindow::loginFailed);
    connect(chatClient, &ChatClient::messageReceived, this, &ChatWindow::messageReceived);
    connect(chatClient, &ChatClient::disconnected, this, &ChatWindow::disconnectedFromServer);
    connect(chatClient, &ChatClient::error, this, &ChatWindow::error);
    connect(chatClient, &ChatClient::userJoined, this, &ChatWindow::userJoined);
    connect(chatClient, &ChatClient::userLeft, this, &ChatWindow::userLeft);
    // connection to server
    connect(ui->connectButton, &QPushButton::clicked, this, &ChatWindow::attemptConnection);
    // connections for send message
    connect(ui->sendButton, &QPushButton::clicked, this, &ChatWindow::sendMessage);
    connect(ui->messageEdit, &QLineEdit::returnPressed, this, &ChatWindow::sendMessage);
}

ChatWindow::~ChatWindow()
{
    delete ui;
    delete chatClient;
    delete chatModel;
}

void ChatWindow::attemptConnection()
{
    const QString hostAddress =
            QInputDialog::getText(this, tr("chose server"), tr("server address"), QLineEdit::Normal, LOCAL_HOST);
    if (hostAddress.isEmpty()) {
        return;
    }
    ui->connectButton->setEnabled(false);
    chatClient->connectToServer(QHostAddress(hostAddress), PORT);
}

void ChatWindow::connectedToServer()
{
    const QString newUsername = QInputDialog::getText(this, tr("chose username"), tr("username"));
    if (newUsername.isEmpty()) {
        return chatClient->disconnectFromHost();
    }
    attemptLogin(newUsername);
}

void ChatWindow::attemptLogin(const QString& userName)
{
    chatClient->login(userName);
}

void ChatWindow::loggedIn()
{
    ui->sendButton->setEnabled(true);
    ui->messageEdit->setEnabled(true);
    ui->chatView->setEnabled(true);

    lastUserName.clear();
}

void ChatWindow::loginFailed(const QString& reason)
{
    QMessageBox::critical(this, tr("Error"), reason);
    connectedToServer();
}

void ChatWindow::messageReceived(const QString& sender, const QString& message)
{
    int newRow = chatModel->rowCount();
    if (lastUserName != sender) {
        lastUserName = sender;

        QFont boldFont;
        boldFont.setBold(true);

        chatModel->insertRows(newRow, 2);
        chatModel->setData(chatModel->index(newRow, 0), sender + QLatin1Char(':'));
        chatModel->setData(chatModel->index(newRow, 0), int(Qt::AlignLeft | Qt::AlignVCenter),
                             Qt::TextAlignmentRole);
        chatModel->setData(chatModel->index(newRow, 0), boldFont, Qt::FontRole);
        ++newRow;
    } else {
        chatModel->insertRow(newRow);
    }
    chatModel->setData(chatModel->index(newRow, 0), message);
    chatModel->setData(chatModel->index(newRow, 0), int(Qt::AlignLeft | Qt::AlignVCenter), Qt::TextAlignmentRole);
    ui->chatView->scrollToBottom();
}

void ChatWindow::sendMessage()
{
    const QString message = ui->messageEdit->text();
    chatClient->sendMessage(message);

    const int newRow = chatModel->rowCount();
    chatModel->insertRow(newRow);
    chatModel->setData(chatModel->index(newRow, 0), message);
    chatModel->setData(chatModel->index(newRow, 0), int(Qt::AlignRight | Qt::AlignVCenter), Qt::TextAlignmentRole);

    ui->messageEdit->clear();
    ui->chatView->scrollToBottom();
    lastUserName.clear();
}

void ChatWindow::disconnectedFromServer()
{
    QMessageBox::warning(this, tr("Disconnected"), tr("The host terminated the connection"));

    ui->sendButton->setEnabled(false);
    ui->messageEdit->setEnabled(false);
    ui->chatView->setEnabled(false);
    ui->connectButton->setEnabled(true);
    lastUserName.clear();
}

void ChatWindow::userEventImpl(const QString& username, const QString& event)
{
    const int newRow = chatModel->rowCount();
    chatModel->insertRow(newRow);
    chatModel->setData(chatModel->index(newRow, 0), tr("%1 %2").arg(username, event));
    chatModel->setData(chatModel->index(newRow, 0), Qt::AlignCenter, Qt::TextAlignmentRole);
    chatModel->setData(chatModel->index(newRow, 0), QBrush(Qt::gray), Qt::ForegroundRole);

    ui->chatView->scrollToBottom();
    lastUserName.clear();
}

void ChatWindow::userJoined(const QString& username)
{
    userEventImpl(username, "joined the group");
}
void ChatWindow::userLeft(const QString& username)
{
    userEventImpl(username, "left the group");
}

void ChatWindow::error(const QAbstractSocket::SocketError socketError)
{
    switch (socketError) {
        case QAbstractSocket::RemoteHostClosedError:
        case QAbstractSocket::ProxyConnectionClosedError:
            return;
        case QAbstractSocket::ConnectionRefusedError:
            QMessageBox::critical(this, tr("Error"), tr("The host refused the connection"));
            break;
        case QAbstractSocket::ProxyConnectionRefusedError:
            QMessageBox::critical(this, tr("Error"), tr("The proxy refused the connection"));
            break;
        case QAbstractSocket::ProxyNotFoundError:
            QMessageBox::critical(this, tr("Error"), tr("Could not find the proxy"));
            break;
        case QAbstractSocket::HostNotFoundError:
            QMessageBox::critical(this, tr("Error"), tr("Could not find the server"));
            break;
        case QAbstractSocket::SocketAccessError:
            QMessageBox::critical(this, tr("Error"), tr("You don't have permissions to execute this operation"));
            break;
        case QAbstractSocket::SocketResourceError:
            QMessageBox::critical(this, tr("Error"), tr("Too many connections opened"));
            break;
        case QAbstractSocket::SocketTimeoutError:
            QMessageBox::warning(this, tr("Error"), tr("Operation timed out"));
            return;
        case QAbstractSocket::ProxyConnectionTimeoutError:
            QMessageBox::critical(this, tr("Error"), tr("Proxy timed out"));
            break;
        case QAbstractSocket::NetworkError:
            QMessageBox::critical(this, tr("Error"), tr("Unable to reach the network"));
            break;
        case QAbstractSocket::UnknownSocketError:
            QMessageBox::critical(this, tr("Error"), tr("An unknown error occurred"));
            break;
        case QAbstractSocket::UnsupportedSocketOperationError:
            QMessageBox::critical(this, tr("Error"), tr("Operation not supported"));
            break;
        case QAbstractSocket::ProxyAuthenticationRequiredError:
            QMessageBox::critical(this, tr("Error"), tr("Your proxy requires authentication"));
            break;
        case QAbstractSocket::ProxyProtocolError:
            QMessageBox::critical(this, tr("Error"), tr("Proxy communication failed"));
            break;
        case QAbstractSocket::TemporaryError:
        case QAbstractSocket::OperationError:
            QMessageBox::warning(this, tr("Error"), tr("Operation failed, please try again"));
            return;
        default:
            Q_UNREACHABLE();
    }

    ui->connectButton->setEnabled(true);
    ui->sendButton->setEnabled(false);
    ui->messageEdit->setEnabled(false);
    ui->chatView->setEnabled(false);
    lastUserName.clear();
}

#include <QStandardItemModel>
#include <QInputDialog>
#include <QMessageBox>
#include <QHostAddress>
#include "clientwindow.h"
#include "ui_window.h"
#include "constants.h"

ClientWindow::ClientWindow(QWidget* parent)
    : QWidget(parent), ui(new Ui::ClientWindow), clientCore(new ClientCore(this)),
      chatModel(new QStandardItemModel(this))
{
    ui->setupUi(this);
    chatModel->insertColumn(0);
    ui->chatView->setModel(chatModel);
    ui->listWidget->setFocusPolicy(Qt::NoFocus);

    // handle signals from logic view of client
    connect(clientCore, &ClientCore::connected, this, &ClientWindow::connectedToServer);
    connect(clientCore, &ClientCore::loggedIn, this, &ClientWindow::loggedIn);
    connect(clientCore, &ClientCore::loginError, this, &ClientWindow::loginFailed);
    connect(clientCore, &ClientCore::messageReceived, this, &ClientWindow::messageReceived);
    connect(clientCore, &ClientCore::disconnected, this, &ClientWindow::disconnectedFromServer);
    connect(clientCore, &ClientCore::error, this, &ClientWindow::error);
    connect(clientCore, &ClientCore::userJoined, this, &ClientWindow::userJoined);
    connect(clientCore, &ClientCore::userLeft, this, &ClientWindow::userLeft);
    // connection to server
    connect(ui->connectButton, &QPushButton::clicked, this, &ClientWindow::attemptConnection);
    // connections for send message
    connect(ui->sendButton, &QPushButton::clicked, this, &ClientWindow::sendMessage);
    connect(ui->messageEdit, &QLineEdit::returnPressed, this, &ClientWindow::sendMessage);
}

ClientWindow::~ClientWindow()
{
    delete ui;
    delete clientCore;
    delete chatModel;
}

void ClientWindow::attemptConnection()
{
    const QString hostAddress =
            QInputDialog::getText(this, tr("choose server"), tr("address"), QLineEdit::Normal, LOCAL_HOST);
    if (hostAddress.isEmpty()) {
        return;
    }
    ui->connectButton->setEnabled(false);
    clientCore->connectToServer(QHostAddress(hostAddress), PORT);
}

void ClientWindow::connectedToServer()
{
    const QString newUsername = QInputDialog::getText(this, tr("choose username"), tr("username"));
    if (newUsername.isEmpty()) {
        return clientCore->disconnectFromHost();
    }
    attemptLogin(newUsername);
}

void ClientWindow::attemptLogin(const QString& userName)
{
    clientCore->login(userName);
}

void ClientWindow::loggedIn()
{
    ui->sendButton->setEnabled(true);
    ui->messageEdit->setEnabled(true);
    ui->chatView->setEnabled(true);

    lastUserName.clear();
}

void ClientWindow::loginFailed(const QString& reason)
{
    QMessageBox::critical(this, tr("Error"), reason);
    connectedToServer();
}

void ClientWindow::messageReceived(const QString& sender, const QString& message)
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

void ClientWindow::sendMessage()
{
    const QString message = ui->messageEdit->text();
    clientCore->sendMessage(message);

    const int newRow = chatModel->rowCount();
    chatModel->insertRow(newRow);
    chatModel->setData(chatModel->index(newRow, 0), message);
    chatModel->setData(chatModel->index(newRow, 0), int(Qt::AlignRight | Qt::AlignVCenter), Qt::TextAlignmentRole);

    ui->messageEdit->clear();
    ui->chatView->scrollToBottom();
    lastUserName.clear();
}

void ClientWindow::disconnectedFromServer()
{
    QMessageBox::warning(this, tr("Disconnected"), tr("The host terminated the connection"));

    ui->sendButton->setEnabled(false);
    ui->messageEdit->setEnabled(false);
    ui->chatView->setEnabled(false);
    ui->connectButton->setEnabled(true);
    lastUserName.clear();
}

void ClientWindow::userEventImpl(const QString& username, const QString& event)
{
    const int newRow = chatModel->rowCount();
    chatModel->insertRow(newRow);
    chatModel->setData(chatModel->index(newRow, 0), tr("%1 %2").arg(username, event));
    chatModel->setData(chatModel->index(newRow, 0), Qt::AlignCenter, Qt::TextAlignmentRole);
    chatModel->setData(chatModel->index(newRow, 0), QBrush(Qt::gray), Qt::ForegroundRole);

    ui->chatView->scrollToBottom();
    lastUserName.clear();
}

void ClientWindow::userJoined(const QString& username)
{
    userEventImpl(username, "joined the group");
    ui->listWidget->addItem(username);
}

void ClientWindow::userLeft(const QString& username)
{
    userEventImpl(username, "left the group");
    QList<QListWidgetItem*> items = ui->listWidget->findItems(username, Qt::MatchExactly);
    if (items.isEmpty()) {
        return;
    }
    delete items.at(0);
}

void ClientWindow::error(const QAbstractSocket::SocketError socketError)
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

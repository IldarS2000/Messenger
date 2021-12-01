#include <QStandardItemModel>
#include <QInputDialog>
#include <QMessageBox>
#include <QFormLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QHostAddress>
#include <QDateTime>
#include "clientwindow.h"
#include "ui_window.h"
#include "constants.h"

ClientWindow::ClientWindow(QWidget* parent)
    : QWidget(parent), ui(new Ui::ClientWindow), clientCore(new ClientCore(this)),
      chatModel(new QStandardItemModel(this))
{
    ui->setupUi(this);
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    setMinimumSize(minWindowWidth, minWindowHeight);

    chatModel->insertColumn(0);
    ui->chatView->setModel(chatModel);
    ui->listWidget->setFocusPolicy(Qt::NoFocus);

    // connect for handle signals from logic view of client
    connect(clientCore, &ClientCore::connected, this, &ClientWindow::connected);
    connect(clientCore, &ClientCore::loggedIn, this, &ClientWindow::loggedIn);
    connect(clientCore, &ClientCore::loginError, this, &ClientWindow::loginError);
    connect(clientCore, &ClientCore::messageReceived, this, &ClientWindow::messageReceived);
    connect(clientCore, &ClientCore::disconnected, this, &ClientWindow::disconnected);
    connect(clientCore, &ClientCore::error, this, &ClientWindow::error);
    connect(clientCore, &ClientCore::userJoined, this, &ClientWindow::userJoined);
    connect(clientCore, &ClientCore::userLeft, this, &ClientWindow::userLeft);
    // connect to server
    connect(ui->connectButton, &QPushButton::clicked, this, &ClientWindow::attemptConnection);
    // connect for send message
    connect(ui->sendButton, &QPushButton::clicked, this, &ClientWindow::sendMessage);
    connect(ui->messageEdit, &QLineEdit::returnPressed, this, &ClientWindow::sendMessage);
}

ClientWindow::~ClientWindow()
{
    delete ui;
    delete clientCore;
    delete chatModel;
}

QString ClientWindow::getTextDialog(const QString& title, const QString& label, const QString& defaultText = "")
{
    QDialog dialog(this);
    dialog.setWindowFlags(dialog.windowFlags() & ~Qt::WindowContextHelpButtonHint);
    dialog.setWindowTitle(title);
    dialog.resize(300, 100);

    QFormLayout form(&dialog);
    form.addRow(new QLabel(label));

    QLineEdit lineEdit(&dialog);
    lineEdit.setText(defaultText);
    form.addRow(&lineEdit);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
    form.addRow(&buttonBox);
    QObject::connect(&buttonBox, SIGNAL(accepted()), &dialog, SLOT(accept()));
    QObject::connect(&buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()));

    if (dialog.exec() == QDialog::Accepted) {
        return lineEdit.text();
    }
    return {};
}

void ClientWindow::attemptConnection()
{
    QString hostAddress = getTextDialog("choose server", "address", LOCAL_HOST);
    if (hostAddress.isEmpty()) {
        return;
    }
    ui->connectButton->setEnabled(false);
    clientCore->connectToServer(QHostAddress(hostAddress), PORT);
}

void ClientWindow::connected()
{
    const QString newUsername = getTextDialog("choose username", "username");
    if (newUsername.isEmpty() || newUsername.size() > maximumUserNameSize) {
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

void ClientWindow::loginError(const QString& reason)
{
    QMessageBox::critical(this, tr("Error"), reason);
    connected();
}

QStringList ClientWindow::splitString(const QString& str, const int rowSize)
{
    QString temp = str;
    QStringList list;
    list.reserve(temp.size() / rowSize + 1);

    while (!temp.isEmpty()) {
        list.append(temp.left(rowSize).trimmed());
        temp.remove(0, rowSize);
    }
    return list;
}

QStringList ClientWindow::splitText(const QString& text)
{
    const QStringList words = text.split(QRegExp("[\r\n\t ]+"), QString::SkipEmptyParts);
    const int wordsCount    = words.size();
    QStringList rows;
    rows.append("");
    for (int i = 0, j = 0; i < wordsCount; ++i) {
        if (words[i].size() > maxMessageRowSize - rows[j].size()) {
            if (words[i].size() > maxMessageRowSize) {
                QStringList bigWords = splitString(words[i], maxMessageRowSize);
                for (const auto& bigWord : bigWords) {
                    if (rows[j].isEmpty()) {
                        rows[j] += bigWord + QString(" ");
                    }
                    rows.append(bigWord + QString(" "));
                    ++j;
                }
            } else {
                rows.append(words[i] + QString(" "));
                ++j;
            }
        } else {
            rows[j] += words[i] + QString(" ");
        }
    }
    return rows;
}

void ClientWindow::displayMessage(const QString& message, const int lastRowNumber, const int alignMask)
{
    QStringList rows    = splitText(message);
    const int rowsCount = rows.size();
    int currentRow      = lastRowNumber;
    const QString time  = QDateTime::currentDateTime().toString("hh:mm");
    for (int i = 0; i < rowsCount; ++i) {
        chatModel->insertRow(currentRow);
        if (i == rowsCount - 1) {
            chatModel->setData(chatModel->index(currentRow, 0), rows[i] + QString(" (") + time + QString(")"));
        } else {
            chatModel->setData(chatModel->index(currentRow, 0), rows[i]);
        }
        chatModel->setData(chatModel->index(currentRow, 0), int(alignMask | Qt::AlignVCenter), Qt::TextAlignmentRole);
        ui->chatView->scrollToBottom();
        ++currentRow;
    }
}

void ClientWindow::messageReceived(const QString& sender, const QString& message)
{
    int currentRow = chatModel->rowCount();
    if (lastUserName != sender) {
        lastUserName = sender;

        QFont boldFont;
        boldFont.setBold(true);

        chatModel->insertRow(currentRow);
        chatModel->setData(chatModel->index(currentRow, 0), sender);
        chatModel->setData(chatModel->index(currentRow, 0), int(Qt::AlignLeft | Qt::AlignVCenter),
                           Qt::TextAlignmentRole);
        chatModel->setData(chatModel->index(currentRow, 0), boldFont, Qt::FontRole);
        ++currentRow;
    }
    displayMessage(message, currentRow, Qt::AlignLeft);
}

void ClientWindow::sendMessage()
{
    const QString message = ui->messageEdit->text();
    if (message.isEmpty() || message.size() > maximumMessageSize) {
        return;
    }
    clientCore->sendMessage(message);

    int currentRow = chatModel->rowCount();
    displayMessage(message, currentRow, Qt::AlignRight);

    ui->messageEdit->clear();
    ui->chatView->scrollToBottom();
    lastUserName.clear();
}

void ClientWindow::disconnected()
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

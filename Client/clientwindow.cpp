#include <QStandardItemModel>
#include <QInputDialog>
#include <QMessageBox>
#include <QFormLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QHostAddress>
#include <QDateTime>
#include <QTimer>
#include "ui_window.h"
#include "clientwindow.h"
#include "constants.h"

ClientWindow::ClientWindow(QWidget* parent)
    : QWidget(parent), ui(new Ui::ClientWindow), clientCore(new ClientCore(this)),
      chatModel(new QStandardItemModel(this)), loadingScreen(new LoadingScreen)
{
    ui->setupUi(this);
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    setMinimumSize(minWindowWidth, minWindowHeight);

    chatModel->insertColumn(0);
    ui->chatView->setModel(chatModel);
    ui->users->setFocusPolicy(Qt::NoFocus);

    // connect for handle signals from logic view of client
    connect(clientCore, &ClientCore::connected, loadingScreen, &LoadingScreen::close);
    connect(clientCore, &ClientCore::connected, this, &ClientWindow::connected);
    connect(clientCore, &ClientCore::loggedIn, this, &ClientWindow::loggedIn);
    connect(clientCore, &ClientCore::loginError, this, &ClientWindow::loginError);
    connect(clientCore, &ClientCore::messageReceived, this, &ClientWindow::messageReceived);
    connect(clientCore, &ClientCore::disconnected, this, &ClientWindow::disconnected);
    connect(clientCore, &ClientCore::error, this, &ClientWindow::error);
    connect(clientCore, &ClientCore::userJoined, this, &ClientWindow::userJoined);
    connect(clientCore, &ClientCore::userLeft, this, &ClientWindow::userLeft);
    connect(clientCore, &ClientCore::informJoiner, this, &ClientWindow::informJoiner);
    // connect for send message
    connect(ui->sendButton, &QPushButton::clicked, this, &ClientWindow::sendMessage);
    connect(ui->messageEdit, &QLineEdit::returnPressed, this, &ClientWindow::sendMessage);

    attemptConnection();
}

ClientWindow::~ClientWindow()
{
    delete ui;
    delete clientCore;
    delete chatModel;
    delete loadingScreen;
}

QPair<QString, QString> ClientWindow::getConnectionCredentials()
{
    QDialog dialog(this);
    dialog.setWindowFlags(dialog.windowFlags() & ~Qt::WindowContextHelpButtonHint & ~Qt::WindowMaximizeButtonHint);
    dialog.setWindowTitle("Connect group");
    dialog.resize(300, 100);
    QFormLayout form(&dialog);

    QLabel usernameLabel("username");
    form.addRow(&usernameLabel);
    QLineEdit usernameLineEdit(&dialog);
    form.addRow(&usernameLineEdit);

    QLabel passwordLabel("password");
    form.addRow(&passwordLabel);
    QLineEdit passwordLineEdit(&dialog);
    passwordLineEdit.setEchoMode(QLineEdit::Password);
    form.addRow(&passwordLineEdit);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
    form.addRow(&buttonBox);

    connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(clientCore, &ClientCore::disconnected, &dialog, &QDialog::close);

    if (dialog.exec() == QDialog::Accepted) {
        return {usernameLineEdit.text(), passwordLineEdit.text()};
    }
    return {};
}

void ClientWindow::attemptConnection()
{
    loadingScreen->show();
    clientCore->connectToServer(QHostAddress(HOST), PORT);
}

void ClientWindow::connected()
{
    QPair<QString, QString> credentials;
    bool usernameOk;
    bool passwordOk;
    do {
        credentials             = getConnectionCredentials();
        const QString& username = credentials.first;
        const QString& password = credentials.second;
        usernameOk =
                (!username.isEmpty() && (username.size() >= minUserNameSize && username.size() <= maxUserNameSize));
        passwordOk =
                (!password.isEmpty() && (password.size() >= minPasswordSize && password.size() <= maxPasswordSize));
        if (!usernameOk) {
            QMessageBox::information(this, tr("username error"),
                                     tr("min %1 characters\nmax %2 characters")
                                             .arg(QString::number(minUserNameSize), QString::number(maxUserNameSize)));
            continue;
        }
        if (!passwordOk) {
            QMessageBox::information(this, tr("password error"),
                                     tr("min %1 characters\nmax %2 characters")
                                             .arg(QString::number(minPasswordSize), QString::number(maxPasswordSize)));
            continue;
        }
        attemptLogin(username, password);
        credentials.first.clear();
        credentials.second.clear();
        return;
    } while (true);
}

void ClientWindow::attemptLogin(const QString& username, const QString& password)
{
    clientCore->login(username, password);
}

void ClientWindow::loggedIn()
{
    ui->sendButton->setEnabled(true);
    ui->messageEdit->setEnabled(true);
    ui->chatView->setEnabled(true);
    QTimer::singleShot(0, ui->messageEdit, SLOT(setFocus()));
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
    if (message.isEmpty() || message.size() > maxMessageSize) {
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
    qWarning() << "The host terminated the connection";

    ui->sendButton->setEnabled(false);
    ui->messageEdit->setEnabled(false);
    ui->chatView->setEnabled(false);
    lastUserName.clear();

    attemptConnection();
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
    ui->users->addItem(username);
}

void ClientWindow::userLeft(const QString& username)
{
    userEventImpl(username, "left the group");
    QList<QListWidgetItem*> items = ui->users->findItems(username, Qt::MatchExactly);
    if (items.isEmpty()) {
        return;
    }
    delete items.at(0);
}

void ClientWindow::informJoiner(const QStringList& usernames)
{
    for (const auto& username : usernames) {
        ui->users->addItem(username);
    }
}

void ClientWindow::error(const QAbstractSocket::SocketError socketError)
{
    switch (socketError) {
        case QAbstractSocket::ConnectionRefusedError:
            qWarning() << "ConnectionRefusedError";
            break;
        case QAbstractSocket::RemoteHostClosedError:
            qWarning() << "RemoteHostClosedError";
            break;
        case QAbstractSocket::HostNotFoundError:
            qWarning() << "HostNotFoundError";
            break;
        case QAbstractSocket::SocketAccessError:
            qWarning() << "SocketAccessError";
            break;
        case QAbstractSocket::SocketResourceError:
            qWarning() << "SocketResourceError";
            break;
        case QAbstractSocket::SocketTimeoutError:
            qWarning() << "SocketTimeoutError";
            return;
        case QAbstractSocket::DatagramTooLargeError:
            qWarning() << "DatagramTooLargeError";
            break;
        case QAbstractSocket::NetworkError:
            qWarning() << "NetworkError";
            break;
        case QAbstractSocket::AddressInUseError:
            qWarning() << "AddressInUseError";
            break;
        case QAbstractSocket::SocketAddressNotAvailableError:
            qWarning() << "SocketAddressNotAvailableError";
            break;
        case QAbstractSocket::UnsupportedSocketOperationError:
            qWarning() << "UnsupportedSocketOperationError";
            break;
        case QAbstractSocket::UnfinishedSocketOperationError:
            qWarning() << "UnfinishedSocketOperationError";
            break;
        case QAbstractSocket::ProxyAuthenticationRequiredError:
            qWarning() << "ProxyAuthenticationRequiredError";
            break;
        case QAbstractSocket::SslHandshakeFailedError:
            qWarning() << "SslHandshakeFailedError";
            break;
        case QAbstractSocket::ProxyConnectionRefusedError:
            qWarning() << "ProxyConnectionRefusedError";
            break;
        case QAbstractSocket::ProxyConnectionClosedError:
            qWarning() << "ProxyConnectionClosedError";
            break;
        case QAbstractSocket::ProxyConnectionTimeoutError:
            qWarning() << "ProxyConnectionTimeoutError";
            break;
        case QAbstractSocket::ProxyNotFoundError:
            qWarning() << "ProxyNotFoundError";
            break;
        case QAbstractSocket::ProxyProtocolError:
            qWarning() << "ProxyProtocolError";
            break;
        case QAbstractSocket::OperationError:
            qWarning() << "OperationError";
            return;
        case QAbstractSocket::SslInternalError:
            qWarning() << "SslInternalError";
            break;
        case QAbstractSocket::SslInvalidUserDataError:
            qWarning() << "SslInvalidUserDataError";
            break;
        case QAbstractSocket::TemporaryError:
            qWarning() << "TemporaryError";
            break;
        default:
            Q_UNREACHABLE();
    }

    ui->sendButton->setEnabled(false);
    ui->messageEdit->setEnabled(false);
    ui->chatView->setEnabled(false);
    lastUserName.clear();
    attemptConnection();
}

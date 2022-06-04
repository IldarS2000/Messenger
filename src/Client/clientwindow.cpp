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
#include "login.h"
#include "register.h"
#include "clientwindow.h"
#include "constants.h"

ClientWindow::ClientWindow(QWidget* parent)
    : QWidget(parent), ui(new Ui::ClientWindow), clientCore(new ClientCore(this)),
      chatModel(new QStandardItemModel(this)), loadingScreen(new LoadingScreen), logged(false), loginWindow(new Login),
      registerWindow(new Register), createGroupWindow(new CreateGroup)
{
    // ui setup
    ui->setupUi(this);
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    setMinimumSize(minWindowWidth, minWindowHeight);
    this->setWindowState(Qt::WindowState::WindowActive);

    chatModel->insertColumn(0);
    ui->chatView->setModel(chatModel);
    ui->users->setFocusPolicy(Qt::NoFocus);

    // connect ui and client core
    connect(clientCore, &ClientCore::connectedSig, loadingScreen, &LoadingScreen::close);
    connect(clientCore, &ClientCore::connectedSig, this, &ClientWindow::connected);
    connect(clientCore, &ClientCore::loggedInSig, this, &ClientWindow::loggedIn);
    connect(clientCore, &ClientCore::registeredSig, this, &ClientWindow::registered);
    connect(clientCore, &ClientCore::loginErrorSig, this, &ClientWindow::loginError);
    connect(clientCore, &ClientCore::registerErrorSig, this, &ClientWindow::registerError);
    connect(clientCore, &ClientCore::messageReceivedSig, this, &ClientWindow::messageReceived);
    connect(clientCore, &ClientCore::disconnectedSig, this, &ClientWindow::disconnected);
    connect(clientCore, &ClientCore::errorSig, this, &ClientWindow::error);
    connect(clientCore, &ClientCore::userJoinedSig, this, &ClientWindow::userJoined);
    connect(clientCore, &ClientCore::userLeftSig, this, &ClientWindow::userLeft);
    connect(clientCore, &ClientCore::informJoinerSig, this, &ClientWindow::informJoiner);
    // connect for send message
    connect(ui->sendButton, &QPushButton::clicked, this, &ClientWindow::sendMessage);
    connect(ui->messageEdit, &QLineEdit::returnPressed, this, &ClientWindow::sendMessage);
    // connect for login
    connect(loginWindow, &Login::signInSig, this, &ClientWindow::signInClicked);
    connect(loginWindow, &Login::signUpSig, this, &ClientWindow::loginSignUpClicked);
    // connect for register
    connect(registerWindow, &Register::signUpSig, this, &ClientWindow::registerSignUpClicked);
    // connect for create group
    connect(ui->createGroup, &QPushButton::clicked, this, &ClientWindow::createGroupClicked);
    // connect for connect to group
    connect(ui->connectGroup, &QPushButton::clicked, this, &ClientWindow::connectGroupClicked);

    // try connect
    QTimer::singleShot(100, this, [this]() { this->attemptConnection(); });
}

ClientWindow::~ClientWindow()
{
    delete ui;
    delete clientCore;
    delete chatModel;
    delete loadingScreen;
    delete loginWindow;
    delete registerWindow;
    delete createGroupWindow;
}

QPair<QString, QString> ClientWindow::getConnectionCredentials()
{
    QDialog dialog(this);
    dialog.setWindowFlags(dialog.windowFlags() & ~Qt::WindowContextHelpButtonHint & ~Qt::WindowMaximizeButtonHint);
    dialog.setWindowTitle("Connect group");
    dialog.resize(300, 100);
    QFormLayout form(&dialog);

    QLabel usernameLabel("group name");
    form.addRow(&usernameLabel);
    QLineEdit groupNameLineEdit(&dialog);
    form.addRow(&groupNameLineEdit);

    QLabel passwordLabel("password");
    form.addRow(&passwordLabel);
    QLineEdit passwordLineEdit(&dialog);
    passwordLineEdit.setEchoMode(QLineEdit::Password);
    form.addRow(&passwordLineEdit);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
    form.addRow(&buttonBox);

    connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(clientCore, &ClientCore::disconnectedSig, &dialog, &QDialog::close);

    if (dialog.exec() == QDialog::Accepted) {
        return {groupNameLineEdit.text(), passwordLineEdit.text()};
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
    loginWindow->show();
    connect(loginWindow, &Login::closeSig, this, &QWidget::close);
}

void ClientWindow::attemptLogin(const QString& username, const QString& password)
{
    clientCore->login(username, password);
}

void ClientWindow::loggedIn()
{
    // enable main window ui
    ui->sendButton->setEnabled(true);
    ui->messageEdit->setEnabled(true);
    ui->chatView->setEnabled(true);
    ui->messageEdit->setFocus();

    // update for logged state
    lastUserName.clear();
    logged = true;

    // close login window
    disconnect(loginWindow, &Login::closeSig, this, &QWidget::close);
    loginWindow->close();
}

void ClientWindow::registered()
{
    QMessageBox::information(this, tr("Register"), tr("you registered successfully"));
    registerWindow->close();
}

void ClientWindow::loginError(const QString& reason)
{
    QMessageBox::critical(this, tr("Error"), reason);
    connected();
}

void ClientWindow::registerError(const QString& reason)
{
    QMessageBox::critical(this, tr("Error"), reason);
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

void ClientWindow::displayMessage(const QString& message, const QString& time, const int lastRowNumber,
                                  const int alignMask)
{
    QStringList rows    = splitText(message);
    const int rowsCount = rows.size();
    int currentRow      = lastRowNumber;
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

void ClientWindow::messageReceived(const Message& message)
{
    int currentRow = chatModel->rowCount();
    if (lastUserName != message.getSender() && clientUserName != message.getSender()) {
        lastUserName = message.getSender();

        QFont boldFont;
        boldFont.setBold(true);

        chatModel->insertRow(currentRow);
        chatModel->setData(chatModel->index(currentRow, 0), message.getSender());
        chatModel->setData(chatModel->index(currentRow, 0), int(Qt::AlignLeft | Qt::AlignVCenter),
                           Qt::TextAlignmentRole);
        chatModel->setData(chatModel->index(currentRow, 0), boldFont, Qt::FontRole);
        ++currentRow;
    }
    if (clientUserName == message.getSender()) {
        displayMessage(message.getMessage(), message.getTime(), currentRow, Qt::AlignRight);
    } else {
        displayMessage(message.getMessage(), message.getTime(), currentRow, Qt::AlignLeft);
    }
}

void ClientWindow::sendMessage()
{
    const QString message = ui->messageEdit->text();
    if (message.isEmpty() || message.size() > maxMessageSize) {
        return;
    }
    const QString time = QDateTime::currentDateTime().toString("hh:mm");
    clientCore->sendMessage(message, time);

    int currentRow = chatModel->rowCount();
    displayMessage(message, time, currentRow, Qt::AlignRight);

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
    logged = false;
    chatModel->removeRows(0, chatModel->rowCount());
    ui->users->clear();
    disconnect(loginWindow, &Login::closeSig, this, &QWidget::close);
    loginWindow->close();
    registerWindow->close();
    if (isVisible()) {
        attemptConnection();
    }
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
    if (logged) {
        userEventImpl(username, "joined the group");
        ui->users->addItem(username);
    }
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

void ClientWindow::informJoiner(const QStringList& usernames, const QList<Message>& messages)
{
    for (const auto& username : usernames) {
        ui->users->addItem(username);
    }
    for (const auto& message : messages) {
        messageReceived(message);
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
    logged = false;
    chatModel->removeRows(0, chatModel->rowCount());
    ui->users->clear();
    disconnect(loginWindow, &Login::closeSig, this, &QWidget::close);
    loginWindow->close();
    registerWindow->close();
    if (isVisible()) {
        attemptConnection();
    }
}

QString ClientWindow::encryptPassword(const QString& password)
{
    return QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Md5);
}

void ClientWindow::signInClicked()
{
    const QString userName          = loginWindow->getName();
    clientUserName                  = userName;
    const QString encryptedPassword = encryptPassword(loginWindow->getPassword());
    attemptLogin(userName, encryptedPassword);
    loginWindow->clearState(); // for security reason clear sensitive info
}

void ClientWindow::loginSignUpClicked()
{
    loginWindow->hide();
    registerWindow->show();
    connect(registerWindow, &Register::closeSig, loginWindow, &QWidget::show);
}

void ClientWindow::attemptRegister(const QString& username, const QString& password)
{
    clientCore->registerUser(username, encryptPassword(password));
}

void ClientWindow::registerSignUpClicked()
{
    const QString userName = registerWindow->getName();
    const QString password = registerWindow->getPassword();
    attemptRegister(userName, password);
    registerWindow->clearState(); // for security reason clear sensitive info
}

void ClientWindow::createGroupClicked()
{
    createGroupWindow->show();
}

void ClientWindow::connectGroupClicked()
{
    const auto& [name, password] = getConnectionCredentials();
}
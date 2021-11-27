#include <QMessageBox>
#include "serverwindow.h"
#include "ui_serverwindow.h"
#include "constants.h"

ServerWindow::ServerWindow(QWidget* parent)
    : QWidget(parent), ui(new Ui::ServerWindow), chatServer(new ServerCore(this))
{
    ui->setupUi(this);
    connect(ui->startStopButton, &QPushButton::clicked, this, &ServerWindow::toggleStartServer);
    connect(chatServer, &ServerCore::logMessage, this, &ServerWindow::logMessage);
}

ServerWindow::~ServerWindow()
{
    delete ui;
    delete chatServer;
}

void ServerWindow::toggleStartServer()
{
    if (chatServer->isListening()) {
        chatServer->stopServer();
        ui->startStopButton->setText(tr("Start Server"));
        logMessage("Server Stopped");
    } else {
        if (!chatServer->listen(QHostAddress::Any, PORT)) {
            QMessageBox::critical(this, tr("Error"), tr("Unable to start the server"));
            return;
        }
        logMessage("Server Started");
        ui->startStopButton->setText(tr("Stop Server"));
    }
}

void ServerWindow::logMessage(const QString& msg)
{
    ui->logEditor->appendPlainText(msg + QLatin1Char('\n'));
}

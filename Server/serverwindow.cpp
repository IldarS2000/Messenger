#include <QMessageBox>
#include "serverwindow.h"
#include "ui_serverwindow.h"
#include "constants.h"

ServerWindow::ServerWindow(QWidget* parent)
    : QWidget(parent), ui(new Ui::ServerWindow), serverCore(new ServerCore(this))
{
    ui->setupUi(this);
    connect(ui->startStopButton, &QPushButton::clicked, this, &ServerWindow::toggleStartServer);
    connect(serverCore, &ServerCore::logMessage, this, &ServerWindow::logMessage);
}

ServerWindow::~ServerWindow()
{
    delete ui;
    delete serverCore;
}

void ServerWindow::toggleStartServer()
{
    if (serverCore->isListening()) {
        serverCore->stopServer();
        ui->startStopButton->setText(tr("Start Server"));
        logMessage("Server Stopped");
    } else {
        if (!serverCore->listen(QHostAddress::Any, PORT)) {
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

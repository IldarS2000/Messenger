#include "login.h"
#include "ui_Login.h"

Login::Login(QWidget* parent) : QWidget(parent), ui(new Ui::Login)
{
    ui->setupUi(this);
    setWindowModality(Qt::ApplicationModal);
    ui->nameLine->setFocus();
    ui->passwordLine->setEchoMode(QLineEdit::Password);

    connect(ui->signInButton, &QPushButton::clicked, this, &Login::saveState);
    connect(ui->signInButton, &QPushButton::clicked, this, &Login::signInClicked);
    connect(ui->signUpButton, &QPushButton::clicked, this, &Login::signUpClicked);
}

Login::~Login()
{
    delete ui;
}

QString Login::getName() const
{
    return name;
}

QString Login::getPassword() const
{
    return password;
}

void Login::signInClicked()
{
    saveState();
    emit signInSig();
}

void Login::signUpClicked()
{
    emit signUpSig();
}

void Login::closeEvent(QCloseEvent* event)
{
    emit closeSig();
}

void Login::clearState()
{
    ui->nameLine->clear();
    ui->passwordLine->clear();
    name.clear();
    password.clear();
}

void Login::saveState()
{
    name     = ui->nameLine->text();
    password = ui->passwordLine->text();
}

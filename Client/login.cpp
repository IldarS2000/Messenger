#include "login.h"
#include "ui_Login.h"

Login::Login(QWidget* parent) : QWidget(parent), ui(new Ui::Login)
{
    ui->setupUi(this);
    setWindowModality(Qt::ApplicationModal);
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

/* blacklist check approach */
bool Login::isValidName(const QString& name)
{
    if (name.isEmpty()) {
        return false;
    }
    if (name.size() > maxUserNameSize) {
        return false;
    }
    const bool isCorrectSymbols =
            std::all_of(name.begin(), name.end(), [](const auto c) { return c.isLetter() || c == '_'; });
    if (!isCorrectSymbols) {
        return false;
    }

    return true;
}

/* blacklist check approach */
bool Login::isValidDurablePassword(const QString& password)
{
    if (password.isEmpty()) {
        return false;
    }
    if (!(password.size() >= minPasswordSize && password.size() <= maxPasswordSize)) {
        return false;
    }
    bool containsUpper      = false;
    bool containsLower      = false;
    bool containsDigit      = false;
    bool containsNotDesired = false;
    for (const auto c : password) {
        if (c.isUpper()) {
            containsUpper = true;
            continue;
        }
        if (c.isLower()) {
            containsLower = true;
            continue;
        }
        if (c.isDigit()) {
            containsDigit = true;
            continue;
        }
        containsNotDesired = true;
    }

    return containsUpper && containsLower && containsDigit && !containsNotDesired;
}

void Login::signInClicked()
{
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

#include <QMessageBox>
#include "register.h"
#include "ui_Register.h"
#include "constants.h"


Register::Register(QWidget* parent) : QWidget(parent), ui(new Ui::Register)
{
    ui->setupUi(this);
    setWindowModality(Qt::ApplicationModal);
    ui->password1Line->setEchoMode(QLineEdit::Password);
    ui->password2Line->setEchoMode(QLineEdit::Password);

    connect(ui->signUpButton, &QPushButton::clicked, this, &Register::signUpClicked);
}

Register::~Register()
{
    delete ui;
}

void Register::closeEvent(QCloseEvent* event)
{
    emit closeSig();
}

QString Register::getName() const
{
    return name;
}

QString Register::getPassword() const
{
    return password;
}

/* blacklist check approach */
bool Register::isValidName(const QString& name)
{
    if (name.isEmpty()) {
        return false;
    }
    if (name.size() > MAX_USER_NAME_SIZE) {
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
bool Register::isValidDurablePassword(const QString& password)
{
    if (password.isEmpty()) {
        return false;
    }
    if (!(password.size() >= MIN_PASSWORD_SIZE && password.size() <= MAX_PASSWORD_SIZE)) {
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

void Register::clearEditLines()
{
    ui->nameLine->clear();
    ui->password1Line->clear();
    ui->password2Line->clear();
}

void Register::clearState()
{
    clearEditLines();
    name.clear();
    password.clear();
}

void Register::saveState()
{
    name     = ui->nameLine->text();
    password = ui->password1Line->text();
}

void Register::signUpClicked()
{
    saveState();
    if (!isValidName(name)) {
        clearEditLines();
        QMessageBox::warning(this, tr("Error"), tr("name can contain only letters and cannot be more than 32 symbols"));
        return;
    }
#ifdef NDEBUG
    if (!isValidDurablePassword(password)) {
        clearEditLines();
        QMessageBox::warning(this, tr("Error"),
                             tr("password is too weak, password should contain A-Z, a-z, 0-9 and should be not "
                                "less than 8 symbols and cannot be more than 32 symbols"));
        return;
    }
#endif
    if (password != ui->password2Line->text()) {
        clearEditLines();
        QMessageBox::warning(this, tr("Error"), tr("passwords not equal"));
        return;
    }

    emit signUpSig();
}

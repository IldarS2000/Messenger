#include <QMessageBox>
#include "constants.h"
#include "creategroup.h"
#include "ui_CreateGroup.h"


CreateGroup::CreateGroup(QWidget* parent) : QWidget(parent), ui(new Ui::CreateGroup)
{
    ui->setupUi(this);
    setWindowModality(Qt::ApplicationModal);
    ui->passwordLine->setEchoMode(QLineEdit::Password);

    connect(ui->createGroup, &QPushButton::clicked, this, &CreateGroup::createGroupClicked);
}

CreateGroup::~CreateGroup()
{
    delete ui;
}

QString CreateGroup::getName() const
{
    return name;
}

QString CreateGroup::getPassword() const
{
    return password;
}

/* blacklist check approach */
bool CreateGroup::isValidName(const QString& name)
{
    if (name.isEmpty()) {
        return false;
    }
    if (name.size() > MAX_GROUP_NAME_SIZE) {
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
bool CreateGroup::isValidDurablePassword(const QString& password)
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

void CreateGroup::clearEditLines()
{
    ui->nameLine->clear();
    ui->passwordLine->clear();
}

void CreateGroup::clearState()
{
    clearEditLines();
    name.clear();
    password.clear();
}

void CreateGroup::saveState()
{
    name = ui->nameLine->text();
#ifdef GROUP_PASSWORD_IMPLEMENTED
    password = ui->passwordLine->text();
#else
    password = "temp";
#endif
}

void CreateGroup::createGroupClicked()
{
    saveState();
    if (!isValidName(name)) {
        clearEditLines();
        QMessageBox::warning(this, tr("Error"), tr("name can contain only letters and cannot be more than 32 symbols"));
        return;
    }
#ifdef GROUP_PASSWORD_IMPLEMENTED
    if (!isValidDurablePassword(password)) {
        clearEditLines();
        QMessageBox::warning(this, tr("Error"),
                             tr("password is too weak, password should contain A-Z, a-z, 0-9 and should be not "
                                "less than 8 symbols and cannot be more than 32 symbols"));
        return;
    }
#endif

    emit createGroupSig();
}
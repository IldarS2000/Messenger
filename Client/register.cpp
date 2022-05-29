#include "register.h"
#include "ui_Register.h"


Register::Register(QWidget* parent) : QWidget(parent), ui(new Ui::Register)
{
    ui->setupUi(this);
    setWindowModality(Qt::ApplicationModal);
    ui->password1Line->setEchoMode(QLineEdit::Password);
    ui->password2Line->setEchoMode(QLineEdit::Password);
}

Register::~Register()
{
    delete ui;
}

void Register::closeEvent(QCloseEvent* event)
{
    emit closeSig();
}

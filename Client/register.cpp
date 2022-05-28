#include "register.h"
#include "ui_Register.h"


Register::Register(QWidget* parent) : QWidget(parent), ui(new Ui::Register)
{
    ui->setupUi(this);
}

Register::~Register()
{
    delete ui;
}

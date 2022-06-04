//
// Created by Ildar on 04.06.2022.
//

// You may need to build the project (run Qt uic code generator) to get "ui_CreateGroup.h" resolved

#include "creategroup.h"
#include "ui_CreateGroup.h"


CreateGroup::CreateGroup(QWidget* parent) : QWidget(parent), ui(new Ui::CreateGroup)
{
    ui->setupUi(this);
}

CreateGroup::~CreateGroup()
{
    delete ui;
}

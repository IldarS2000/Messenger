#include <QMovie>
#include <QLabel>
#include <QStyle>
#include "loadingscreen.h"
#include "ui_LoadingScreen.h"


LoadingScreen::LoadingScreen(QWidget* parent)
    : QWidget(parent), ui(new Ui::LoadingScreen), movie(new QMovie(":/loading.gif"))
{
    ui->setupUi(this);
    setWindowFlags(Qt::Window);
    setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);
    setWindowModality(Qt::ApplicationModal);
    ui->animation->setMovie(movie);
    movie->start();
}

LoadingScreen::~LoadingScreen()
{
    delete ui;
    delete movie;
}

void LoadingScreen::closeEvent(QCloseEvent* event)
{
    emit closeSig();
}
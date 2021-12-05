#ifndef MESSENGER_LOADINGSCREEN_H
#define MESSENGER_LOADINGSCREEN_H

#include <QWidget>
#include <QMovie>

QT_BEGIN_NAMESPACE
namespace Ui {
    class LoadingScreen;
}
QT_END_NAMESPACE

class LoadingScreen : public QWidget
{
    Q_OBJECT

public:
    explicit LoadingScreen(QWidget* parent = nullptr);
    ~LoadingScreen() override;

private:
    Ui::LoadingScreen* ui;
    QMovie* movie;
};

#endif // MESSENGER_LOADINGSCREEN_H

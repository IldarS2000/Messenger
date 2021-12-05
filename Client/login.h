#ifndef MESSENGER_LOGIN_H
#define MESSENGER_LOGIN_H

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
    class Login;
}
QT_END_NAMESPACE

class Login : public QWidget
{
    Q_OBJECT

public:
    explicit Login(QWidget* parent = nullptr);
    ~Login() override;

private:
    Ui::Login* ui;
};

#endif // MESSENGER_LOGIN_H

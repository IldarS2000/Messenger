#ifndef LOGIN_H
#define LOGIN_H

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
    [[nodiscard]] QString getName() const;
    [[nodiscard]] QString getPassword() const;
    void clearState();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void saveState();

private slots:
    void signInClicked();
    void signUpClicked();

signals:
    void signInSig();
    void signUpSig();
    void closeSig();

private:
    Ui::Login* ui;
    QString name;
    QString password;
};

#endif // LOGIN_H

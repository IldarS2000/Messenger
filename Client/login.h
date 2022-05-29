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
    [[nodiscard]] static bool isValidName(const QString& name);
    [[nodiscard]] static bool isValidDurablePassword(const QString& password);
    void clearState();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void signInClicked();
    void signUpClicked();
    void saveState();

signals:
    void signInSig();
    void signUpSig();
    void closeSig();

private:
    Ui::Login* ui;
    QString name;
    QString password;
    static constexpr int maxUserNameSize = 32;
    static constexpr int minPasswordSize = 8;
    static constexpr int maxPasswordSize = 32;
};

#endif // LOGIN_H

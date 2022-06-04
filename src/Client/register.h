#ifndef REGISTER_H
#define REGISTER_H

#include <QWidget>


QT_BEGIN_NAMESPACE
namespace Ui {
    class Register;
}
QT_END_NAMESPACE

class Register : public QWidget
{
    Q_OBJECT

public:
    explicit Register(QWidget* parent = nullptr);
    ~Register() override;
    [[nodiscard]] QString getName() const;
    [[nodiscard]] QString getPassword() const;
    [[nodiscard]] static bool isValidName(const QString& name);
    [[nodiscard]] static bool isValidDurablePassword(const QString& password);
    void clearState();

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void clearEditLines();
    void saveState();

private slots:
    void signUpClicked();

signals:
    void signUpSig();
    void closeSig();

private:
    Ui::Register* ui;
    QString name;
    QString password;
};


#endif // REGISTER_H

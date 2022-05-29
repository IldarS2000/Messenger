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

protected:
    void closeEvent(QCloseEvent* event) override;

signals:
    void signUpSig();
    void closeSig();

private:
    Ui::Register* ui;
};


#endif // REGISTER_H

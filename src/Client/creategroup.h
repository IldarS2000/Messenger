#ifndef CREATEGROUP_H
#define CREATEGROUP_H

#include <QWidget>


QT_BEGIN_NAMESPACE
namespace Ui {
    class CreateGroup;
}
QT_END_NAMESPACE

class CreateGroup : public QWidget
{
    Q_OBJECT

public:
    explicit CreateGroup(QWidget* parent = nullptr);
    ~CreateGroup() override;
    [[nodiscard]] QString getName() const;
    [[nodiscard]] QString getPassword() const;
    [[nodiscard]] static bool isValidName(const QString& name);
    [[nodiscard]] static bool isValidDurablePassword(const QString& password);
    void clearState();

private:
    void clearEditLines();
    void saveState();

private slots:
    void createGroupClicked();

signals:
    void createGroupSig();

private:
    Ui::CreateGroup* ui;
    QString name;
    QString password;
};


#endif // CREATEGROUP_H

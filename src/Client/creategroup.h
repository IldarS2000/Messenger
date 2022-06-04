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

private:
    Ui::CreateGroup* ui;
};


#endif // CREATEGROUP_H

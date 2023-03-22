#ifndef ELIDELINEEDIT_H
#define ELIDELINEEDIT_H

#include <QLineEdit>

class ElideLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    ElideLineEdit(QWidget *parent = nullptr);
};

#endif // ELIDELINEEDIT_H

#ifndef ELIDELINEEDIT_H
#define ELIDELINEEDIT_H

#include "UI/widgets/klineedit.h"

class ElideLineEdit : public KLineEdit
{
    Q_OBJECT
public:
    ElideLineEdit(QWidget *parent = nullptr);
};

#endif // ELIDELINEEDIT_H

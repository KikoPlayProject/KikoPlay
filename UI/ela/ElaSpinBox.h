#ifndef ELASPINBOX_H
#define ELASPINBOX_H

#include <QSpinBox>

#include "stdafx.h"

class ElaSpinBoxPrivate;
class ElaSelfTheme;
class ElaSpinBox : public QSpinBox
{
    Q_OBJECT
    Q_Q_CREATE(ElaSpinBox)
public:
    explicit ElaSpinBox(QWidget* parent = nullptr);
    ~ElaSpinBox();
    void setSelfTheme(ElaSelfTheme *selfTheme);
    QLineEdit *getLineEdit();

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;
};

#endif // ELASPINBOX_H

#ifndef ELACHECKBOX_H
#define ELACHECKBOX_H

#include <QCheckBox>

#include "stdafx.h"
class ElaCheckBox : public QCheckBox
{
    Q_OBJECT
    Q_PROPERTY_CREATE(int, BorderRadius)
public:
    explicit ElaCheckBox(QWidget* parent = nullptr);
    explicit ElaCheckBox(const QString& text, QWidget* parent = nullptr);
    ~ElaCheckBox();
};

#endif // ELACHECKBOX_H

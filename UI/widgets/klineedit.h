#ifndef KLINEEDIT_H
#define KLINEEDIT_H

#include <QLineEdit>

class KLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    explicit KLineEdit(QWidget *parent = nullptr);
    explicit KLineEdit(const QString &contents, QWidget *parent = nullptr);

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;
};

#endif // KLINEEDIT_H

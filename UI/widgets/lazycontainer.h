#ifndef LAZYCONTAINER_H
#define LAZYCONTAINER_H

#include <QWidget>

class LazyContainer : public QWidget
{
    Q_OBJECT
public:
    using InitFunc = std::function<QWidget *()>;
    LazyContainer(QWidget *parent, QLayout *l, InitFunc initFunc);
    void init();
private:
    InitFunc contentInitFunc;
    bool inited = false;
    QLayout *contentLayout = nullptr;
    // QWidget interface
protected:
    void showEvent(QShowEvent *event);
    void paintEvent(QPaintEvent *event);
};

#endif // LAZYCONTAINER_H

#ifndef DANMUVIEW_H
#define DANMUVIEW_H
#include "framelessdialog.h"
#include <QLineEdit>
#include "Play/Danmu/common.h"
class QTreeView;
class QActionGroup;
class DanmuFilterBox : public QLineEdit
{
    Q_OBJECT
public:
    explicit DanmuFilterBox(QWidget *parent = nullptr);
signals:
    void filterChanged(int type,const QString &filterStr);
private:
    QActionGroup *filterTypeGroup;
};
class DanmuView : public CFramelessDialog
{
    Q_OBJECT
public:
    explicit DanmuView(const QList<DanmuComment *> *danmuList, QWidget *parent = nullptr);
    explicit DanmuView(const QList<QSharedPointer<DanmuComment> > *danmuList, QWidget *parent = nullptr);

private:
    QTreeView *danmuView;
    DanmuFilterBox *filterEdit;
    void initView(int danmuCount);

    // CFramelessDialog interface
protected:
    virtual void onClose();
};



#endif // DANMUVIEW_H

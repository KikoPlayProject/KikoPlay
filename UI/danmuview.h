#ifndef DANMUVIEW_H
#define DANMUVIEW_H
#include "framelessdialog.h"
#include <QLineEdit>
#include "Play/Danmu/common.h"
class QTreeView;
class QActionGroup;
class QLabel;
class DanmuFilterBox : public QLineEdit
{
    Q_OBJECT
public:
    enum FilterType
    {
        CONTENT = 0,
        USER,
        TYPE,
        TIME
    };
    explicit DanmuFilterBox(QWidget *parent = nullptr);
    void setFilter(FilterType type, const QString &filterStr);

signals:
    void filterChanged(int type,const QString &filterStr);
private:
    QActionGroup *filterTypeGroup;
};
class DanmuView : public CFramelessDialog
{
    Q_OBJECT
public:
    explicit DanmuView(const QList<DanmuComment *> *danmuList, QWidget *parent = nullptr,
                       const QString &filterStr="", DanmuFilterBox::FilterType type=DanmuFilterBox::FilterType::USER);
    explicit DanmuView(const QList<QSharedPointer<DanmuComment> > *danmuList, QWidget *parent = nullptr,
                       const QString &filterStr="", DanmuFilterBox::FilterType type=DanmuFilterBox::FilterType::USER);

private:
    QTreeView *danmuView;
    DanmuFilterBox *filterEdit;
    QLabel *tipLabel;
    void initView(int danmuCount);

    // CFramelessDialog interface
protected:
    virtual void onClose();
};



#endif // DANMUVIEW_H

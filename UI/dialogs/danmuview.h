#ifndef DANMUVIEW_H
#define DANMUVIEW_H
#include "UI/framelessdialog.h"
#include "Play/Danmu/common.h"
#include "UI/widgets/klineedit.h"
class QTreeView;
class QActionGroup;
class QLabel;
class DanmuFilterBox : public KLineEdit
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
    explicit DanmuView(const QVector<DanmuComment *> *danmuList, QWidget *parent = nullptr,
                       int sourceId=-1);
    explicit DanmuView(const QVector<QSharedPointer<DanmuComment> > *danmuList, QWidget *parent = nullptr,
                       int sourceId=-1);

private:
    QTreeView *danmuView;
    DanmuFilterBox *filterEdit;
    QLabel *tipLabel;
    void initView();
};



#endif // DANMUVIEW_H

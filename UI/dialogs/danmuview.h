#ifndef DANMUVIEW_H
#define DANMUVIEW_H
#include "UI/framelessdialog.h"
#include "Play/Danmu/common.h"
#include "UI/widgets/klineedit.h"
class QTreeView;
class QActionGroup;
class QToolButton;
class QComboBox;
class QLabel;
class DanmuViewProxyModel;
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
protected:
    void resizeEvent(QResizeEvent *event) override;
private:
    QActionGroup *filterTypeGroup;
    QToolButton *optionsButton = nullptr;
    void updateFilterField();
    void layoutFieldButton();
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
    DanmuViewProxyModel *proxyModel;
    DanmuFilterBox *filterEdit;
    QComboBox *typeCombo;
    QLabel *countLabel;
    QLabel *totalLabel;
    QLabel *senderLabel;
    QLabel *typeLabels[3];
    QLabel *emptyLabel;
    QLabel *detailLabel;
    QLabel *detailMetaLabel;
    QWidget *detailPanel;
    void initView();
    void initStats(int originCount);
    void updateDetails();
};



#endif // DANMUVIEW_H

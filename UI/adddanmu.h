#ifndef ADDDANMU_H
#define ADDDANMU_H

#include "framelessdialog.h"
#include <QStyledItemDelegate>
#include "Play/Danmu/Provider/info.h"
#include "Play/Danmu/common.h"

class QStackedLayout;
class QToolButton;
class QComboBox;
class QListWidget;
class QTreeWidget;
class PlayListItem;
class SearchItemWidget:public QWidget
{
    Q_OBJECT
public:
    SearchItemWidget(DanmuSourceItem *item);
signals:
    void addSearchItem(DanmuSourceItem *item);
private:
    DanmuSourceItem searchItem;
public:
    virtual QSize sizeHint() const;
};
class AddDanmu : public CFramelessDialog
{
    Q_OBJECT
public:
    explicit AddDanmu(const PlayListItem *item, QWidget *parent = nullptr, bool autoPauseVideo=true);
    QList<QPair<DanmuSourceInfo,QList<DanmuComment *> > > selectedDanmuList;
private:
    QToolButton *onlineDanmuPage,*urlDanmuPage,*selectedDanmuPage;
    QLineEdit *keywordEdit,*urlEdit;
    QPushButton *searchButton, *addUrlButton;
    QComboBox *sourceCombo;
    QListWidget *searchResultWidget;
    const QStringList sourceList=QString("Bilibili;Dandan;Tucao;5dm").split(';');
    QTreeWidget *selectedDanmuWidget;
    QString providerId;

    void search();
    void addSearchItem(DanmuAccessResult *result);
    void addURL();
    QWidget *setupSearchPage();
    QWidget *setupURLPage();
    QWidget *setupSelectedPage();
    int processCounter;
    void beginProcrss();
    void endProcess();

protected:
    virtual void onAccept();
    virtual void onClose();

    // QObject interface
public:
    virtual bool eventFilter(QObject *watched, QEvent *event);
};

#endif // ADDDANMU_H

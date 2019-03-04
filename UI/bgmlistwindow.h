#ifndef BGMLISTWINDOW_H
#define BGMLISTWINDOW_H

#include <QWidget>
class BgmList;
class QTreeView;
class QPushButton;
class BgmListWindow : public QWidget
{
    Q_OBJECT
public:
    explicit BgmListWindow(QWidget *parent = nullptr);
private:
    QTreeView *bgmListView;
    BgmList *bgmList;
    QList<QPushButton *> weekDayBtnList;
    QStringList btnTitles={tr("Sun"),tr("Mon"),tr("Tue"),tr("Wed"),tr("Thu"),tr("Fri"),tr("Sat"),tr("All")};

    int weekDay;
signals:
    void searchBgm(const QString &item);
public slots:

    // QWidget interface
protected:
    virtual void showEvent(QShowEvent *event);
    virtual void resizeEvent(QResizeEvent *event);
};

#endif // BGMLISTWINDOW_H

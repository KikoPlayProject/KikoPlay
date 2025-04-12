#ifndef TIMELINEEDIT_H
#define TIMELINEEDIT_H

#include "UI/framelessdialog.h"
#include <QAbstractItemModel>
#include "Play/Danmu/danmupool.h"
class TimeLineInfoModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit TimeLineInfoModel(QVector<QPair<int, int>> *timelines, QObject *parent=nullptr);
    void addSpace(int start,int duration);
    void removeSpace(const QModelIndex &index);
    inline const QVector<QPair<int,int>> *getTimeLine(){return timelineInfo;}
private:
    QVector<QPair<int,int>> *timelineInfo;
    // QAbstractItemModel interface
public:
    inline virtual QModelIndex index(int row, int column, const QModelIndex &parent) const{return parent.isValid()?QModelIndex():createIndex(row,column);}
    inline virtual QModelIndex parent(const QModelIndex &) const{return QModelIndex();}
    inline virtual int rowCount(const QModelIndex &parent) const{return parent.isValid()?0:timelineInfo->count();}
    inline virtual int columnCount(const QModelIndex &parent) const {return parent.isValid()?0:3;}
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
};
class SimpleDanumPool : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit SimpleDanumPool(const QVector<SimpleDanmuInfo> &sDanmuList, QObject *parent=nullptr);
    inline const QVector<SimpleDanmuInfo> *getDanmuList(){return &simpleDanmuList;}
    QModelIndex getIndex(int time);
    void refreshTimeline(const QVector<QPair<int, int> > &timelineInfo);
private:
    QVector<SimpleDanmuInfo> simpleDanmuList;
    // QAbstractItemModel interface
public:
    inline virtual QModelIndex index(int row, int column, const QModelIndex &parent) const{return parent.isValid()?QModelIndex():createIndex(row,column);}
    inline virtual QModelIndex parent(const QModelIndex &) const{return QModelIndex();}
    inline virtual int rowCount(const QModelIndex &parent) const{return parent.isValid()?0:simpleDanmuList.count();}
    inline virtual int columnCount(const QModelIndex &parent) const {return parent.isValid()?0:3;}
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
};
class TimeLineBar : public QWidget
{
    Q_OBJECT
public:
    explicit TimeLineBar(const QVector<SimpleDanmuInfo> *sDanmuList, TimeLineInfoModel *timelineModel, QWidget *parent=nullptr);
    void updateInfo();
private:
    const QVector<SimpleDanmuInfo> *simpleDanmuList;
    TimeLineInfoModel *timelineModel;
    StatisInfo statisInfo;
    int duration;
    int currentState;
    int mouseTimeStartPos,mouseTimeEndPos;
    int xPos;
    void refreshStatisInfo();
protected:
    virtual void paintEvent(QPaintEvent *event);
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mouseDoubleClickEvent(QMouseEvent *event);
    virtual void enterEvent(QEnterEvent *event);
    virtual void leaveEvent(QEvent *event);
    virtual void keyPressEvent(QKeyEvent *event);
signals:
    void mouseMove(int x, int time, bool isStart);
    void mouseLeave();
    void mousePress(int time);
    void addSpace(int start,int duration);
};
class TimelineEdit : public CFramelessDialog
{
    Q_OBJECT
public:
    TimelineEdit(const DanmuSource *source, const QVector<SimpleDanmuInfo> &simpleDanmuList, QWidget *parent = nullptr,int curTime=-1);

    QVector<QPair<int,int>> timelineInfo;
private:
    TimeLineInfoModel *timelineModel;
    int tmpStartTime;
};

#endif // TIMELINEEDIT_H

#ifndef TIMELINEEDIT_H
#define TIMELINEEDIT_H

#include "UI/framelessdialog.h"
#include "UI/widgets/timelinebar.h"
#include "Play/Danmu/common.h"
#include <QAbstractTableModel>

class QTreeView;
class QLineEdit;
class QPushButton;
class QLabel;

class TimeLineInfoModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit TimeLineInfoModel(int delay, QObject *parent = nullptr);
    void setRules(QVector<TimelineMarker> rules);
    void setPendingOffset(int start, int delta);
    const QVector<TimelineMarker> &rules() const { return items; }
    int rowCount(const QModelIndex &parent = {}) const override { return parent.isValid() ? 0 : items.size(); }
    int columnCount(const QModelIndex &parent = {}) const override { return parent.isValid() ? 0 : 3; }
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
private:
    QVector<TimelineMarker> items;
    int baseDelay;
    int pendingStart = 0, pendingDelta = 0;
};

class NearbyDanmuModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    using QAbstractTableModel::QAbstractTableModel;
    void refresh(const QVector<SimpleDanmuInfo> &comments, const QVector<DanmuTimeResult> &times, qint64 focus);
    int rowCount(const QModelIndex &parent = {}) const override { return parent.isValid() ? 0 : rows.size(); }
    int columnCount(const QModelIndex &parent = {}) const override { return parent.isValid() ? 0 : 3; }
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
private:
    struct Row { DanmuTimeResult time; QString text; };
    QVector<Row> rows;
};

class TimelineEdit : public CFramelessDialog
{
    Q_OBJECT
public:
    TimelineEdit(const DanmuSource *source, const QVector<SimpleDanmuInfo> &comments, QWidget *parent = nullptr, int curTime = -1);
    QVector<QPair<int, int>> timelineInfo;
protected:
    void onAccept() override;
private:
    void selectRule(int id);
    void beginNew(int start = -1);
    void edit();
    void addRule();
    void deleteRule();
    void refreshPreview();
    void setError(const QString &message);

    DanmuSource draft;
    QVector<SimpleDanmuInfo> comments;
    QVector<DanmuTimeResult> times;
    QVector<qint64> originalTimes;
    TimeLineInfoModel *timelineModel;
    NearbyDanmuModel *nearbyModel;
    TimeLineBar *timelineBar;
    QTreeView *timelineView;
    QLineEdit *startEdit, *deltaEdit;
    QLabel *editTitle, *errorLabel, *nearbyLabel, *emptyLabel;
    QPushButton *addButton, *deleteButton;
    int selectedId = -1, nextId = 0;
    qint64 focus = 0, sourceEnd = 0;
    bool adding = true, pendingValid = false;
    TimelineMarker pending{-2, 0, 0, 0};
};

#endif // TIMELINEEDIT_H

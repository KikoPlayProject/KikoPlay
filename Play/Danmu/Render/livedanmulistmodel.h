#ifndef LIVEDANMULISTMODEL_H
#define LIVEDANMULISTMODEL_H

#include <QAbstractItemModel>
#include <deque>
#include "../common.h"

class LiveDanmuListModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit LiveDanmuListModel(QObject *parent = nullptr);

    void addLiveDanmu(const QVector<DrawTask> &danmuList);
    void clear();
    QSharedPointer<DanmuComment> getDanmu(const QModelIndex &index);
    void setMaxNum(int m) { maxNum = m; }
    void setShowSender(bool show);
    bool isShowSender() const { return showSender; }
    void setFontFamily(const QString &fontFamily) { danmuFont.setFamily(fontFamily); }
    void setFontSize(int size);
    int getFontSize() const { return danmuFont.pointSize(); }
    void setAlpha(float a) { fontAlpha = 255 * a; }
    void setRandomColor(bool on) { isRandomColor = on; }
    void setMergeCountPos(int pos) { mergeCountPos = pos; }
    void setEnlargeMerged(bool on) { enlargeMerged = on; }
    void setAlignment(Qt::Alignment alignment);
    Qt::Alignment alignment() const { return static_cast<Qt::Alignment>(align); }
signals:
    void danmuAppend();
    // QAbstractItemModel interface
public:
    QModelIndex index(int row, int column, const QModelIndex &parent) const { return parent.isValid()? QModelIndex() : createIndex(row,column); }
    QModelIndex parent(const QModelIndex &child) const { return QModelIndex(); }
    int rowCount(const QModelIndex &parent) const { return liveDanmus.size(); }
    int columnCount(const QModelIndex &parent) const { return parent.isValid()? 0 : 1; }
    QVariant data(const QModelIndex &index, int role) const;
private:
    std::deque<QPair<QSharedPointer<DanmuComment>, QColor>> liveDanmus;
    int maxNum = 100;
    bool showSender = true;
    int fontAlpha = 255;
    int mergeCountPos = 1;
    bool enlargeMerged = true;
    int align = Qt::AlignLeft;
    QFont danmuFont;
    bool isRandomColor = false;
};

#endif // LIVEDANMULISTMODEL_H

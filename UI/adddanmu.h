#ifndef ADDDANMU_H
#define ADDDANMU_H

#include "framelessdialog.h"
#include <QStyledItemDelegate>
#include "Play/Danmu/common.h"

class QStackedLayout;
class QToolButton;
class QComboBox;
class QListWidget;
class QTreeView;
class PlayListItem;
class AddDanmu;
class SearchItemWidget:public QWidget
{
    Q_OBJECT
public:
    SearchItemWidget(const DanmuSource &item);
    DanmuSource source;
signals:
    void addSearchItem(SearchItemWidget *widgetItem);
public:
    virtual QSize sizeHint() const;
};

class PoolComboDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    PoolComboDelegate(const QStringList &pools, QObject *parent = nullptr):QStyledItemDelegate(parent),poolList(pools){}

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;

    inline void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &) const override
    {
        editor->setGeometry(option.rect);
    }
private:
    QStringList poolList;
};
class DanmuItemModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit DanmuItemModel(AddDanmu *dmDialog, bool hasPool, const QString &normalPool, QObject *parent = nullptr);
    void addItem(const DanmuSource &src);
    enum class Columns
    {
        TITLE,
        COUNT,
        PROVIDER,
        DURATION,
        DANMUPOOL
    };

private:
    struct ItemInfo
    {
        QString title;
        QString duration;
        QString provider;
        int count;
    };
    QList<ItemInfo> items;
    QStringList *danmuToPoolList;
    QList<bool> *danmuCheckedList;
    QList<QPair<DanmuSource,QList<DanmuComment *>>> *selectedDanmuList;
    bool hasPoolInfo;
    QString nPool;
public:
    inline virtual QModelIndex index(int row, int column, const QModelIndex &parent) const{return parent.isValid()?QModelIndex():createIndex(row,column);}
    inline virtual QModelIndex parent(const QModelIndex &) const {return QModelIndex();}
    inline virtual int rowCount(const QModelIndex &parent) const {return parent.isValid()?0:items.count();}
    inline virtual int columnCount(const QModelIndex &) const{return hasPoolInfo?5:4;}
    virtual Qt::DropActions supportedDropActions() const{return Qt::MoveAction;}
    virtual QStringList mimeTypes() const { return QStringList()<<"application/x-kikoplayitem"; }
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role);
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const;
    virtual QMimeData *mimeData(const QModelIndexList &indexes) const;
    virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);

};
class RelWordWidget : public QWidget
{
    Q_OBJECT
public:
    explicit RelWordWidget(QWidget *parent=nullptr);
    void setRelWordList(const QStringList &relWords);
    bool isEmpty() const {return relBtns.isEmpty();}
private:
    QList<QPushButton *> relBtns;
signals:
    void relWordClicked(const QString &relWord);
    // QWidget interface
public:
    virtual QSize sizeHint() const;
};

class AddDanmu : public CFramelessDialog
{
    Q_OBJECT
public:
    explicit AddDanmu(const PlayListItem *item, QWidget *parent = nullptr, bool autoPauseVideo=true, const QStringList &poolList=QStringList());
    QList<QPair<DanmuSource,QList<DanmuComment *>>> selectedDanmuList;
    QList<bool> danmuCheckedList;
    QStringList danmuToPoolList;
private:
    QToolButton *onlineDanmuPage,*urlDanmuPage,*selectedDanmuPage;
    QLineEdit *keywordEdit,*urlEdit;
    QPushButton *searchButton, *addUrlButton;
    QComboBox *sourceCombo;
    QListWidget *searchResultWidget;
    QTreeView *selectedDanmuView;
    QString providerId;
    QStringList danmuPools;
    DanmuItemModel *danmuItemModel;
    RelWordWidget *relWordWidget;
    QString themeWord;

    void search();
    void addSearchItem(QList<DanmuSource> &sources);
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

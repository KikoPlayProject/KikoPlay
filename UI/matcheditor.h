#ifndef MATCHEDITOR_H
#define MATCHEDITOR_H

#include "framelessdialog.h"
#include <QAbstractItemModel>
#include <QStyledItemDelegate>

class PlayListItem;
struct MatchInfo;
class QStackedLayout;
class QToolButton;
class QLineEdit;
class QTreeWidget;
class MatchEditor;
class EpComboDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    EpComboDelegate(QObject *parent = nullptr):QStyledItemDelegate(parent){}
    void setEpList(const QStringList &eps) {epList = eps;}

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;

    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &) const override
    {
        editor->setGeometry(option.rect);
    }
private:
    QStringList epList;
};
class EpModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    EpModel(MatchEditor *matchEditor, QObject *parent = nullptr);
    void resetEpList(const QStringList &eps);
    void toggleCheckAll();
    enum class Columns
    {
        FILETITLE,
        EPNAME
    };

private:
    QStringList fileTitles;
    QStringList *batchEp;
    QList<bool> *epCheckedList;
    QStringList headers={tr("FileTitle"),tr("EpName")};
public:
    inline virtual QModelIndex index(int row, int column, const QModelIndex &parent) const{return parent.isValid()?QModelIndex():createIndex(row,column);}
    inline virtual QModelIndex parent(const QModelIndex &) const {return QModelIndex();}
    inline virtual int rowCount(const QModelIndex &parent) const {return parent.isValid()?0:fileTitles.count();}
    inline virtual int columnCount(const QModelIndex &) const{return headers.size();}
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role);
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const;
};
class MatchEditor : public CFramelessDialog
{
    Q_OBJECT
public:
    explicit MatchEditor(const PlayListItem *item, QList<const PlayListItem *> *batchItems = nullptr, QWidget *parent = nullptr);
    ~MatchEditor();

    MatchInfo *matchInfo;
    QString batchAnime;
    QStringList batchEp, animeEps;
    QList<bool> epCheckedList;
    QList<const PlayListItem *> *batchItems;

private:
    const PlayListItem *curItem;
    QToolButton *searchPage,*customPage, *batchPage;
    QStackedLayout *contentStackLayout;
    QLineEdit *animeEdit,*subtitleEdit;
    QTreeWidget *searchResult;

    QWidget *setupSearchPage();
    QWidget *setupCustomPage();
    QWidget *setupBatchPage();

    // CFramelessDialog interface
protected:
    virtual void onAccept();
    virtual void onClose();
};

#endif // MATCHEDITOR_H

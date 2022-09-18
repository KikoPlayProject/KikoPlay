#ifndef RESSEARCHWINDOW_H
#define RESSEARCHWINDOW_H
#include <QWidget>
#include <QAbstractItemModel>
#include "Script/resourcescript.h"

class QTreeView;
class QLabel;
class QLineEdit;
class SearchListModel;
class QPushButton;
class QComboBox;
class ScriptSearchOptionPanel;
class SearchListModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit SearchListModel(QObject *parent = nullptr):QAbstractItemModel(parent){}
    void setList(const QString &curScriptId, QList<ResourceItem> &nList);
    ResourceItem getItem(const QModelIndex &index) const {return resultList.value(index.row());}
    QStringList getMagnetList(const QModelIndexList &indexes);
    enum class Columns
    {
        TIME, TITLE, SIZE
    };
signals:
    void fetching(bool on);
private:
    QString scriptId;
    QList<ResourceItem> resultList;
    QStringList headers{tr("Time"), tr("Title"), tr("Size")};
public:
    inline virtual QModelIndex index(int row, int column, const QModelIndex &parent) const{return parent.isValid()?QModelIndex():createIndex(row,column);}
    inline virtual QModelIndex parent(const QModelIndex &) const {return QModelIndex();}
    inline virtual int rowCount(const QModelIndex &parent) const {return parent.isValid()?0:resultList.count();}
    inline virtual int columnCount(const QModelIndex &parent) const{return parent.isValid()?0:headers.size();}
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
};
class ResSearchWindow : public QWidget
{
    Q_OBJECT
public:
    explicit ResSearchWindow(QWidget *parent = nullptr);
    void search(const QString &keyword, bool setSearchEdit=false);

private:
    QTreeView *searchListView;
    QLabel *totalPageTip;
    QLineEdit *searchEdit,*pageEdit;
    QPushButton *prevPage, *nextPage;
    QComboBox *scriptCombo;
    SearchListModel *searchListModel;
    ScriptSearchOptionPanel *scriptOptionPanel;
    int totalPage,currentPage;
    bool isSearching;
    QString currentKeyword,currentScriptId;
    void setEnable(bool on);
    void pageTurning(int page);
signals:
    void addTask(const QStringList &urls);
protected:
    virtual void resizeEvent(QResizeEvent *event);
};

#endif // RESSEARCHWINDOW_H

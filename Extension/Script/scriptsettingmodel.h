#ifndef SCRIPTSETTINGMODEL_H
#define SCRIPTSETTINGMODEL_H
#include <QAbstractItemModel>
#include <QSharedPointer>
#include <QStyledItemDelegate>
#include "scriptbase.h"
class SettingDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    SettingDelegate(QObject *parent = nullptr):QStyledItemDelegate(parent){}

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;

    inline void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &) const override
    {
        editor->setGeometry(option.rect);
    }
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QSize size = QStyledItemDelegate::sizeHint(option, index);
        size.setHeight(size.height() + 12);
        return size;
    }
};

struct SettingTreeItem
{
    SettingTreeItem(SettingTreeItem * p = nullptr) : parent(p)
    {
        if(parent) parent->subItems.append(this);
    }
    ~SettingTreeItem()
    {
        if(item) delete item;
        qDeleteAll(subItems);
    }
    SettingTreeItem *parent = nullptr;
    QString groupTitle;
    ScriptBase::ScriptSettingItem *item = nullptr;
    int row = 0;
    QVector<SettingTreeItem *> subItems;
};

class ScriptSettingModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    ScriptSettingModel(QSharedPointer<ScriptBase> script, QObject *parent = nullptr);
    enum struct Columns
    {
        TITLE, DESC, VALUE, NONE
    };
    bool settingHasGroup() const {return hasGroup;}
public:
    virtual QModelIndex index(int row, int column, const QModelIndex &parent) const;
    virtual QModelIndex parent(const QModelIndex &index) const;
    virtual int rowCount(const QModelIndex &parent) const;
    virtual int columnCount(const QModelIndex &) const{return (int)Columns::NONE;}
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role);
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const;
signals:
    void itemChanged(const QString &key, int index,  const QString &val);
private:
    void buildSettingTree(QSharedPointer<ScriptBase> script);
    QSharedPointer<SettingTreeItem> rootItem;
    bool hasGroup;
};

#endif // SCRIPTSETTINGMODEL_H

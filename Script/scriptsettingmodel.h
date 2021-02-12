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
public:
    inline virtual QModelIndex index(int row, int column, const QModelIndex &parent) const{return parent.isValid()?QModelIndex():createIndex(row,column);}
    inline virtual QModelIndex parent(const QModelIndex &) const {return QModelIndex();}
    inline virtual int rowCount(const QModelIndex &parent) const {return parent.isValid()?0:settingItems.count();}
    inline virtual int columnCount(const QModelIndex &parent) const{return parent.isValid()?0:(int)Columns::NONE;}
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role);
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const;
signals:
    void itemChanged(const QString &key, int index,  const QString &val);
private:
    QList<ScriptBase::ScriptSettingItem> settingItems;
};

#endif // SCRIPTSETTINGMODEL_H

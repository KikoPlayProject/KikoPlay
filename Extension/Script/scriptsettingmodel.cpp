#include "scriptsettingmodel.h"
#include <QComboBox>
#include <QLineEdit>
#include <QFont>
#include "UI/widgets/elidelineedit.h"
#include "globalobjects.h"
#define ItemChoiceRole Qt::UserRole+1


ScriptSettingModel::ScriptSettingModel(QSharedPointer<ScriptBase> script, QObject *parent) : QAbstractItemModel(parent), hasGroup(false)
{
    rootItem.reset(new SettingTreeItem);
    beginResetModel();
    buildSettingTree(script);
    endResetModel();
}

QModelIndex ScriptSettingModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent)) return QModelIndex();

    SettingTreeItem *parentItem = nullptr;
    parentItem = parent.isValid() ? static_cast<SettingTreeItem *>(parent.internalPointer()) : rootItem.data();

    SettingTreeItem *childItem = parentItem->subItems.at(row);
    return childItem? createIndex(row, column, childItem) : QModelIndex();
}

QModelIndex ScriptSettingModel::parent(const QModelIndex &index) const
{
    if (!index.isValid()) return QModelIndex();

    SettingTreeItem *childItem = static_cast<SettingTreeItem*>(index.internalPointer());
    SettingTreeItem *parentItem = childItem->parent;

    if (parentItem == rootItem) return QModelIndex();

    int parentRow = parentItem->parent->subItems.indexOf(parentItem);
    return createIndex(parentRow, 0, parentItem);
}

int ScriptSettingModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0) return 0;
    SettingTreeItem *parentItem = nullptr;
    parentItem = parent.isValid()? static_cast<SettingTreeItem*>(parent.internalPointer()) : rootItem.data();
    return parentItem->subItems.size();
}

QVariant ScriptSettingModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid()) return QVariant();
    SettingTreeItem *settingItem = static_cast<SettingTreeItem*>(index.internalPointer());
    Columns col=static_cast<Columns>(index.column());
    switch (role)
    {
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
    {
        switch (col)
        {
        case Columns::TITLE:
            if(settingItem->item)
                return settingItem->item->title;
            return settingItem->groupTitle;
        case Columns::DESC:
            if(settingItem->item)
                return settingItem->item->description;
            break;
        case Columns::VALUE:
            if(settingItem->item)
                return settingItem->item->value;
            break;
        default:
            break;
        }
    }
        break;
    case ItemChoiceRole:
        if(settingItem->item)
            return settingItem->item->choices;
        break;
    case Qt::FontRole:
        if(!settingItem->item)
        {
            QFont groupFont(GlobalObjects::normalFont, 15);
            groupFont.setBold(true);
            return groupFont;
        }
        break;
    }
    return QVariant();
}

bool ScriptSettingModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Columns col=static_cast<Columns>(index.column());
    SettingTreeItem *settingItem = static_cast<SettingTreeItem*>(index.internalPointer());
    if(col==Columns::VALUE && settingItem->item)
    {
        settingItem->item->value = value.toString();
        emit itemChanged(settingItem->item->key, settingItem->row, value.toString());
    }
    return true;
}

QVariant ScriptSettingModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    static QStringList headers({tr("Title"), tr("Description"),tr("Value")});
    if (role == Qt::DisplayRole&&orientation == Qt::Horizontal)
    {
        if(section<headers.size()) return headers.at(section);
    }
    return QVariant();
}

Qt::ItemFlags ScriptSettingModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags defaultFlags = QAbstractItemModel::flags(index);
    Columns col=static_cast<Columns>(index.column());
    SettingTreeItem *settingItem = static_cast<SettingTreeItem*>(index.internalPointer());
    if (index.isValid() && col==Columns::VALUE && settingItem->item)
    {
        return  Qt::ItemIsEditable | defaultFlags;
    }
    return defaultFlags;
}

void ScriptSettingModel::buildSettingTree(QSharedPointer<ScriptBase> script)
{
    const QVector<ScriptBase::ScriptSettingItem> &settings = script->settings();
    QMap<QString, SettingTreeItem *> groupParents;
    int row = 0;
    for(const ScriptBase::ScriptSettingItem &item : settings)
    {
        SettingTreeItem *treeItem = nullptr, *parent = rootItem.data();
        if(!item.group.isEmpty())
        {
            hasGroup = true;
            parent = groupParents.value(item.group, nullptr);
            if(!parent)
            {
                parent = new SettingTreeItem(rootItem.get());
                parent->groupTitle = item.group;
                groupParents[item.group] = parent;
            }
        }
        treeItem = new SettingTreeItem(parent);
        treeItem->item = new ScriptBase::ScriptSettingItem(item);
        treeItem->row = row++;
    }
}


QWidget *SettingDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    ScriptSettingModel::Columns col=static_cast<ScriptSettingModel::Columns>(index.column());
    if(col == ScriptSettingModel::Columns::VALUE)
    {
        QString choices = index.data(ItemChoiceRole).toString();
        if(!choices.isEmpty())
        {
            QComboBox *combo=new QComboBox(parent);
            combo->setFrame(false);
            combo->addItems(choices.split(',', Qt::SkipEmptyParts));
            return combo;
        }
    }
    return new ElideLineEdit(parent);
}

void SettingDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    ScriptSettingModel::Columns col=static_cast<ScriptSettingModel::Columns>(index.column());
    if(col == ScriptSettingModel::Columns::VALUE)
    {
        QString choices = index.data(ItemChoiceRole).toString();
        QString value = index.data().toString();
        if(!choices.isEmpty())
        {
            QComboBox *combo = static_cast<QComboBox*>(editor);
            combo->setCurrentIndex(choices.split(',').indexOf(value));
        }
        else
        {
            QLineEdit *lineEdit = static_cast<QLineEdit *>(editor);
            lineEdit->setText(value);
        }
    }
}

void SettingDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    ScriptSettingModel::Columns col=static_cast<ScriptSettingModel::Columns>(index.column());
    if(col == ScriptSettingModel::Columns::VALUE)
    {
        QString choices = index.data(ItemChoiceRole).toString();
        if(!choices.isEmpty())
        {
            QComboBox *combo = static_cast<QComboBox*>(editor);
            model->setData(index,combo->currentText(),Qt::EditRole);
        }
        else
        {
            QStyledItemDelegate::setModelData(editor,model,index);
        }
    }
}

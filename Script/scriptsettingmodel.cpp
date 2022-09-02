#include "scriptsettingmodel.h"
#include <QComboBox>
#include <QLineEdit>
#include "UI/widgets/elidelineedit.h"
#define ItemChoiceRole Qt::UserRole+1


ScriptSettingModel::ScriptSettingModel(QSharedPointer<ScriptBase> script, QObject *parent) : QAbstractItemModel(parent)
{
    beginResetModel();
    settingItems =  script->settings();
    endResetModel();
}

QVariant ScriptSettingModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid()) return QVariant();
    auto &settingItem = settingItems.at(index.row());
    Columns col=static_cast<Columns>(index.column());
    switch (role)
    {
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
    {
        switch (col)
        {
        case Columns::TITLE:
            return settingItem.title;
        case Columns::DESC:
            return settingItem.description;
        case Columns::VALUE:
            return settingItem.value;
        default:
            break;
        }
    }
        break;
    case ItemChoiceRole:
        return settingItem.choices;
    }
    return QVariant();
}

bool ScriptSettingModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Columns col=static_cast<Columns>(index.column());
    if(col==Columns::VALUE)
    {
        auto &settingItem = settingItems[index.row()];
        settingItem.value = value.toString();
        emit itemChanged(settingItem.key, index.row(), value.toString());
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
    if (index.isValid() && col==Columns::VALUE)
    {
        return  Qt::ItemIsEditable | defaultFlags;
    }
    return defaultFlags;
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

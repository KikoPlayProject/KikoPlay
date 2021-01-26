#include "scriptmodel.h"
#include "globalobjects.h"
#define ScriptTypeRole Qt::UserRole+1

ScriptModel::ScriptModel(QObject *parent) : QAbstractItemModel(parent)
{
    beginResetModel();
    for(int i=0;i<ScriptType::UNKNOWN_STYPE;++i)
    {
        auto type = ScriptType(i);
        for(const auto &s: GlobalObjects::scriptManager->scripts(type))
        {
            scriptList.append({type, s->id(), s->name(), s->version(), s->desc(), s->getValue("path")});
        }
    }
    endResetModel();
    QObject::connect(GlobalObjects::scriptManager, &ScriptManager::scriptChanged, this,
                     [this](ScriptType type, const QString &id, ScriptManager::ScriptChangeState state)
    {
        if(state==ScriptManager::ScriptChangeState::ADD)
        {
            beginInsertRows(QModelIndex(), scriptList.size(), scriptList.size());
            auto s = GlobalObjects::scriptManager->getScript(id);
            scriptList.append({type, s->id(), s->name(), s->version(), s->desc(), s->getValue("path")});
            endInsertRows();
        }
        else if(state==ScriptManager::ScriptChangeState::REMOVE)
        {
            auto iter = std::find_if(scriptList.begin(), scriptList.end(), [=](const ScriptInfo &si){return si.id==id;});
            if(iter!=scriptList.end())
            {
                int index = iter - scriptList.begin();
                beginRemoveRows(QModelIndex(), index, index);
                scriptList.erase(iter);
                endRemoveRows();
            }
        }
    });
}

QVariant ScriptModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();
    ScriptModel::Columns col=static_cast<Columns>(index.column());
    const ScriptInfo &script=scriptList.at(index.row());
    if(role==Qt::DisplayRole)
    {
        switch (col)
        {
        case Columns::TYPE:
            if(script.type != ScriptType::UNKNOWN_STYPE)
                return scriptTypes[script.type];
            break;
        case Columns::ID:
            return script.id;
        case Columns::NAME:
            return script.name;
        case Columns::VERSION:
            return script.version;
        case Columns::DESC:
            return script.desc;
        default:
            break;
        }
    }
    else if(role == ScriptTypeRole)
    {
        return script.type;
    }
    return QVariant();
}

QVariant ScriptModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    static QStringList headers({tr("Type"), tr("Id"),tr("Name"),tr("Version"),tr("Description")});
    if (role == Qt::DisplayRole&&orientation == Qt::Horizontal)
    {
        if(section<headers.size()) return headers.at(section);
    }
    return QVariant();
}

void ScriptProxyModel::setType(int type)
{
    ctype = type;
    invalidateFilter();
}

bool ScriptProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    if(ctype==-1) return true;
    QModelIndex index = sourceModel()->index(source_row, static_cast<int>(ScriptModel::Columns::ID), source_parent);
    return ctype == index.data(ScriptTypeRole).value<ScriptType>();
}

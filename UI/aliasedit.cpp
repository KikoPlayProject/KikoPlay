#include "aliasedit.h"
#include <QAbstractItemModel>
#include "MediaLibrary/animeworker.h"
#include "Common/notifier.h"
#include <QListView>
#include <QHBoxLayout>
#include <QAction>

AliasModel::AliasModel(const QString &animeName, QObject *parent):QAbstractItemModel(parent), anime(animeName)
{
    beginResetModel();
    aliasList = AnimeWorker::instance()->getAlias(animeName);
    endResetModel();
}

QModelIndex AliasModel::addAlias()
{
    beginInsertRows(QModelIndex(), aliasList.size(), aliasList.size());
    aliasList.append("");
    endInsertRows();
    return index(aliasList.size()-1, 0, QModelIndex());
}

void AliasModel::removeAlias(const QModelIndex &index)
{
    beginRemoveRows(QModelIndex(), index.row(), index.row());
    AnimeWorker::instance()->removeAlias(anime, aliasList[index.row()], true);
    aliasList.removeAt(index.row());
    endRemoveRows();
}

QVariant AliasModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid()) return QVariant();
    if(role == Qt::DisplayRole)
    {
        return aliasList[index.row()];
    }
    return QVariant();
}

bool AliasModel::setData(const QModelIndex &index, const QVariant &value, int)
{
    if(!index.isValid() || anime.isEmpty()) return false;
    QString alias = value.toString();
    auto remove = [=](int row){
        beginRemoveRows(QModelIndex(), row, row);
        aliasList.removeAt(row);
        endRemoveRows();
    };
    if(alias.isEmpty())
    {
        remove(index.row());
        return false;
    }
    if(alias == anime)
    {
        remove(index.row());
        emit addFailed();
        return false;
    }
    if(AnimeWorker::instance()->hasAnime(alias))
    {
        remove(index.row());
        emit addFailed();
        return false;
    }
    if(AnimeWorker::instance()->addAlias(anime, alias))
    {
        aliasList[index.row()] = alias;
        return true;
    }
    else
    {
        remove(index.row());
        emit addFailed();
        return false;
    }
}

AliasEdit::AliasEdit(const QString &animeName, QWidget *parent) : CFramelessDialog(tr("Alias Edit"),parent)
{
    AliasModel *aliasModel = new AliasModel(animeName, this);
    QObject::connect(aliasModel, &AliasModel::addFailed, this, [=](){
        showMessage(tr("Add Alias Failed: The alias is illegal"), NM_HIDE | NM_ERROR);
    });
    QListView *aliasView = new QListView(this);
    aliasView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    aliasView->setContextMenuPolicy(Qt::ActionsContextMenu);
    aliasView->setSelectionMode(QAbstractItemView::SingleSelection);
    aliasView->setModel(aliasModel);
    QAction *actAddAlias = new QAction(tr("Add Alias"), aliasView);
    QObject::connect(actAddAlias, &QAction::triggered, this, [=](){
        auto index = aliasModel->addAlias();
        aliasView->scrollTo(index);
        aliasView->edit(index);
    });
    QAction *actRemoveAlias = new QAction(tr("Remove Alias"), aliasView);
    QObject::connect(actRemoveAlias, &QAction::triggered, this, [=](){
        auto selection = aliasView->selectionModel()->selectedRows();
        if(selection.size()==0) return;
        aliasModel->removeAlias(selection.first());
    });
    aliasView->addAction(actAddAlias);
    aliasView->addAction(actRemoveAlias);
    QHBoxLayout *aliasHLayout = new QHBoxLayout(this);
    aliasHLayout->setContentsMargins(0, 0, 0, 0);
    aliasHLayout->addWidget(aliasView);
}

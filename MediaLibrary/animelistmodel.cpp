#include "animelistmodel.h"
#include "animemodel.h"
#include "animeworker.h"
#include "Common/notifier.h"
#include "Script/scriptmanager.h"
#include "animeprovider.h"
#include "labelmodel.h"
#include "globalobjects.h"

AnimeListModel::AnimeListModel(AnimeModel *srcAnimeModel, QObject *parent) : QAbstractItemModel(parent),
    animeModel(srcAnimeModel)
{
    beginResetModel();
    checkStatus.fill(Qt::CheckState::Unchecked, animeModel->animes.size());
    endResetModel();
}

void AnimeListModel::updateCheckedInfo()
{
    int totalCount = checkedCount();
    if(totalCount == 0) return;
    cleanState();
    int currentCount = 0;
    for(int i = 0; i < checkStatus.size(); ++i)
    {
        if(checkStatus[i] != Qt::CheckState::Checked) continue;
        Anime *currentAnime = animeModel->animes[i];
        ++currentCount;
        if(currentAnime->scriptId().isEmpty()) continue;
        QSharedPointer<ScriptBase> script = GlobalObjects::scriptManager->getScript(currentAnime->scriptId());
        if(!script) continue;
        showMessage(tr("[%1/%2]Updating %3 from %4").
                    arg(QString::number(currentCount), QString::number(totalCount), currentAnime->name(), script->name()),
                    NM_PROCESS | NM_DARKNESS_BACK);
        Anime *nAnime = new Anime;
        ScriptState state = GlobalObjects::animeProvider->getDetail(currentAnime->toLite(), nAnime);
        if(!state)
        {
            stateHash[i] = state.info;
            delete nAnime;
        }
        else
        {
            AnimeWorker::instance()->addAnime(currentAnime, nAnime);
            stateHash[i] = tr("Update Successfully");
        }
    }
    showMessage(tr("Update Done"), NotifyMessageFlag::NM_HIDE);
    refreshState();
}

void AnimeListModel::updateCheckedTag()
{
    int totalCount = checkedCount();
    if(totalCount == 0) return;
    cleanState();
    int currentCount = 0;
    for(int i = 0; i < checkStatus.size(); ++i)
    {
        if(checkStatus[i] != Qt::CheckState::Checked) continue;
        Anime *currentAnime = animeModel->animes[i];
        ++currentCount;
        if(currentAnime->scriptId().isEmpty()) continue;
        QSharedPointer<ScriptBase> script = GlobalObjects::scriptManager->getScript(currentAnime->scriptId());
        if(!script) continue;
        showMessage(tr("[%1/%2]Fetching Tags %3 from %4").
                    arg(QString::number(currentCount), QString::number(totalCount), currentAnime->name(), script->name()),
                    NM_PROCESS | NM_DARKNESS_BACK);
        QStringList tags;
        ScriptState state = GlobalObjects::animeProvider->getTags(currentAnime, tags);
        if(!state)
        {
            stateHash[i] = state.info;
        }
        else
        {
            if(tags.size()>0)
            {
                LabelModel::instance()->addCustomTags(currentAnime->name(), tags);
            }
            stateHash[i] = tr("Fetch Tags Successfully");
        }
    }
    showMessage(tr("Fetch Done"), NotifyMessageFlag::NM_HIDE);
    refreshState();
}

void AnimeListModel::removeChecked()
{
    int totalCount = checkedCount();
    if(totalCount == 0) return;
    cleanState();
    int currentCount = 0;
    for(int i = checkStatus.size()-1; i>=0; --i)
    {
        if(checkStatus[i] != Qt::CheckState::Checked) continue;
        showMessage(tr("[%1/%2]Removing...").
                    arg(QString::number(++currentCount), QString::number(totalCount)),
                    NM_PROCESS | NM_DARKNESS_BACK);
        beginRemoveRows(QModelIndex(), i, i);
        checkStatus.removeAt(i);
        animeModel->deleteAnime(animeModel->index(i, 0, QModelIndex()));
        endRemoveRows();
    }
    showMessage(tr("Remove Done"), NotifyMessageFlag::NM_HIDE);
}

QVariant AnimeListModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid() || index.row()>=checkStatus.size()
            || index.row()>=animeModel->animes.size()) return QVariant();
    Anime *anime = animeModel->animes[index.row()];
    Columns col=static_cast<Columns>(index.column());
    switch (role)
    {
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
    {
        switch (col)
        {
        case Columns::TITLE:
            return anime->name();
        case Columns::AIR_DATE:
            return anime->airDate();
        case Columns::ADD_TIME:
            return anime->addTimeStr();
        case Columns::SCRIPT:
        {
            auto script = GlobalObjects::scriptManager->getScript(anime->scriptId());
            return script? script->name():anime->scriptId();
        }
        case Columns::STATE:
            if(stateHash.contains(index.row()))
                return stateHash[index.row()];
        default:
            break;
        }
    }
        break;
    case Qt::CheckStateRole:
    {
        if(col==Columns::TITLE)
            return checkStatus[index.row()];
        break;
    }
    }
    return QVariant();
}

QVariant AnimeListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole&&orientation == Qt::Horizontal)
    {
        if(section < headers.size())return headers.at(section);
    }
    return QVariant();
}

Qt::ItemFlags AnimeListModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags defaultFlags = QAbstractItemModel::flags(index);
    if (index.isValid() && index.column()==0)
    {
        return  Qt::ItemIsUserCheckable | defaultFlags;
    }
    return defaultFlags;
}

bool AnimeListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if(role==Qt::CheckStateRole && index.column()==0)
    {
        checkStatus[index.row()] = Qt::CheckState(value.toInt());
        return true;
    }
    return false;
}

void AnimeListModel::cleanState()
{
    QList<int> rows = stateHash.keys();
    stateHash.clear();
    for(int r : rows)
    {
        QModelIndex index = this->index(r, static_cast<int>(Columns::STATE), QModelIndex());
        emit dataChanged(index, index);
    }
}

void AnimeListModel::refreshState()
{
    for(auto iter = stateHash.cbegin(); iter != stateHash.cend(); ++iter)
    {
        QModelIndex index = this->index(iter.key(), static_cast<int>(Columns::STATE), QModelIndex());
        emit dataChanged(index, index);
    }
}

int AnimeListModel::checkedCount()
{
    int totalCount = 0;
    for(int i = 0; i < checkStatus.size(); ++i)
    {
        if(checkStatus[i] != Qt::CheckState::Checked) continue;
        ++totalCount;
    }
    return totalCount;
}

void AnimeListModel::setAllChecked(bool on)
{
    for(int i = 0; i < checkStatus.size(); ++i)
    {
        checkStatus[i] = on? Qt::CheckState::Checked : Qt::CheckState::Unchecked;
        QModelIndex index = this->index(i, static_cast<int>(Columns::STATE), QModelIndex());
        emit dataChanged(index, index);
    }
}

bool AnimeListProxyModel::lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const
{
    static QCollator comparer;
    if(sortColumn()==0)
        return comparer.compare(source_left.data(sortRole()).toString(),source_right.data(sortRole()).toString())<0;
    return QSortFilterProxyModel::lessThan(source_left,source_right);
}

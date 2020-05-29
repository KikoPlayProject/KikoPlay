#include "episodesmodel.h"
#include <QSqlQuery>
#include <QSqlRecord>
#include <QFileDialog>
#include <QComboBox>
#include <QLineEdit>
#include "globalobjects.h"
#include "Play/Video/mpvplayer.h"
EpisodesModel::EpisodesModel(Anime *anime, QObject *parent) : QAbstractItemModel(parent),
    currentAnime(anime),episodeChanged(false)
{

}

void EpisodesModel::setAnime(Anime *anime)
{
    beginResetModel();
    currentAnime = anime;
    endResetModel();
}

void EpisodesModel::updatePath(const QString &oldPath, const QString &newPath)
{
    QSqlQuery query(QSqlDatabase::database("Bangumi_M"));
    query.prepare("update eps set LocalFile=? where Anime=? and LocalFile=?");
    query.bindValue(0,newPath);
    query.bindValue(1,currentAnime->title);
    query.bindValue(2,oldPath);
    query.exec();
}

void EpisodesModel::updateTitle(const QString &path, const QString &title)
{
    QSqlQuery query(QSqlDatabase::database("Bangumi_M"));
    query.prepare("update eps set Name=? where Anime=? and LocalFile=?");
    query.bindValue(0,title);
    query.bindValue(1,currentAnime->title);
    query.bindValue(2,path);
    query.exec();
}

void EpisodesModel::addEpisode(const QString &title, const QString &path)
{
    Episode ep;
    ep.name=title;
    ep.localFile=path;
    int insertPosition = currentAnime->eps.count();
    beginInsertRows(QModelIndex(), insertPosition, insertPosition);
    currentAnime->eps.append(ep);
    endInsertRows();
    episodeChanged=true;
    QSqlQuery query(QSqlDatabase::database("Bangumi_M"));
    query.prepare("insert into eps(Anime,Name,LocalFile) values(?,?,?)");
    query.bindValue(0,currentAnime->title);
    query.bindValue(1,title);
    query.bindValue(2,path);
    query.exec();
}

void EpisodesModel::removeEpisodes(const QModelIndexList &removeIndexes)
{
    QSqlDatabase db=QSqlDatabase::database("Bangumi_M");
    QSqlQuery query(db);
    query.prepare("delete from eps where Anime=? and LocalFile=?");
    db.transaction();
    QList<int> rows;
    for (const QModelIndex &index : removeIndexes)
    {
        if (index.isValid())
        {
            int row=index.row();
            rows.append(row);
            query.bindValue(0,currentAnime->title);
            query.bindValue(1,currentAnime->eps.at(row).localFile);
            query.exec();
        }
    }
    db.commit();
    std::sort(rows.rbegin(),rows.rend());
    for(auto iter=rows.begin();iter!=rows.end();++iter)
    {
        beginRemoveRows(QModelIndex(), *iter, *iter);
        currentAnime->eps.removeAt(*iter);
        endRemoveRows();
    }
    episodeChanged=true;
}

QVariant EpisodesModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid()) return QVariant();
    const Episode &ep=currentAnime->eps.at(index.row());
    int col=index.column();
    switch (role)
    {
    case Qt::DisplayRole:
    {
        if(col==0)
        {
            return ep.name;
        }
        else if(col==2)
        {
            return ep.localFile;
        }
        else if(col==1)
        {
            return ep.lastPlayTime;
        }
        break;
    }
    }
    return QVariant();
}

bool EpisodesModel::setData(const QModelIndex &index, const QVariant &value, int )
{
    int row=index.row(),col=index.column();
    Episode &ep=currentAnime->eps[row];
    QString val=value.toString().trimmed();
    if(val.isEmpty())return false;
    switch (col)
    {
    case 0:
        if(val==ep.name)return false;
        ep.name=value.toString();
        updateTitle(ep.localFile,ep.name);
        episodeChanged=true;
        break;
    case 2:
        if(val==ep.localFile)return false;
        updatePath(ep.localFile,value.toString());
        ep.localFile=value.toString();
        episodeChanged=true;
        break;
    }
    emit dataChanged(index,index);
    return true;
}

QVariant EpisodesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole&&orientation == Qt::Horizontal)
    {
        if(section<3)return headers.at(section);
    }
    return QVariant();
}

QWidget *EpItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    int col=index.column();
    if(col==0)
    {
        QComboBox *combo=new QComboBox(parent);
        combo->setFrame(false);
        combo->setEditable(true);
        combo->addItems(*epNames);
        return combo;
    }
    return QStyledItemDelegate::createEditor(parent,option,index);
}

void EpItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    int col=index.column();
    if(col==0)
    {
        QComboBox *combo = static_cast<QComboBox*>(editor);
        combo->setCurrentText(index.data(Qt::DisplayRole).toString());
    }
    else if(col==2)
    {
        QString file = QFileDialog::getOpenFileName(nullptr,tr("Select Media File"),"",
                                                    QString("Video Files(%1);;All Files(*) ").arg(GlobalObjects::mpvplayer->videoFileFormats.join(" ")));
        QLineEdit *lineEdit=static_cast<QLineEdit *>(editor);
        if(!file.isNull())
        {
            lineEdit->setText(file);
        }
        else
        {
            lineEdit->setText(index.data(Qt::DisplayRole).toString());
        }
    }
}

void EpItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    int col=index.column();
    if(col==0)
    {
        QComboBox *combo = static_cast<QComboBox*>(editor);
        model->setData(index,combo->currentText(),Qt::EditRole);
        return;
    }
    QStyledItemDelegate::setModelData(editor,model,index);
}

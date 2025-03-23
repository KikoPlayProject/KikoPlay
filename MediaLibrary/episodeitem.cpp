#include "episodeitem.h"
#include "UI/widgets/klineedit.h"
#include "episodesmodel.h"
#include "Common/notifier.h"
#include "Common/lrucache.h"
#include "animeprovider.h"
#include "Play/Video/mpvplayer.h"
#include "globalobjects.h"
#include <QComboBox>
#include <QStandardItem>
#include <QLineEdit>
#include <QGridLayout>
#include <QAbstractItemView>
#include <QFileDialog>
#include <QDateTimeEdit>

QWidget *EpItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    EpisodesModel::Columns col = EpisodesModel::Columns(index.column());
    if (col == EpisodesModel::Columns::FINISHTIME)
    {
        QDateTimeEdit *dateTimeEdit = new QDateTimeEdit(parent);
        dateTimeEdit->setDisplayFormat("yyyy-MM-dd hh:mm:ss");
        return dateTimeEdit;
    }
    else if (col == EpisodesModel::Columns::TITLE)
    {
        return nullptr;
    }
    else if (col == EpisodesModel::Columns::LOCALFILE)
    {
        return new KLineEdit(parent);
    }
    return QStyledItemDelegate::createEditor(parent,option,index);
}

void EpItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    if (!curAnime) return;
    EpisodesModel::Columns col=EpisodesModel::Columns(index.column());
    switch (col)
    {
    case EpisodesModel::Columns::LOCALFILE:
    {
        QString file = QFileDialog::getOpenFileName(nullptr,tr("Select Media File"),"",
                                                    QString("Video Files(%1);;All Files(*) ").arg(GlobalObjects::mpvplayer->videoFileFormats.join(" ")));
        QLineEdit *lineEdit = static_cast<QLineEdit *>(editor);
        lineEdit->setText(file.isNull() ? index.data(Qt::DisplayRole).toString() : file);
        break;
    }
    case EpisodesModel::Columns::FINISHTIME:
    {
        QDateTimeEdit *dateTimeEdit = static_cast<QDateTimeEdit *>(editor);
        auto ep = index.data(EpisodesModel::EpRole).value<EpInfo>();
        dateTimeEdit->setDateTime(QDateTime::fromSecsSinceEpoch(ep.finishTime));
        break;
    }
    default:
        break;
    }
}

void EpItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    if (!curAnime) return;
    EpisodesModel::Columns col = EpisodesModel::Columns(index.column());
    switch (col)
    {
    case EpisodesModel::Columns::FINISHTIME:
    {
        QDateTimeEdit *dateTimeEdit = static_cast<QDateTimeEdit *>(editor);
        model->setData(index, dateTimeEdit->dateTime().toSecsSinceEpoch());
        break;
    }
    default:
        QStyledItemDelegate::setModelData(editor,model,index);
    }
}

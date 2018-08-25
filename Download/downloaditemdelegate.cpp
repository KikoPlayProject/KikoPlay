#include "downloaditemdelegate.h"
#include <QApplication>
#include <QProgressBar>
#include "downloadmodel.h"
DownloadItemDelegate::DownloadItemDelegate(QObject *parent) : QStyledItemDelegate(parent)
{

}

void DownloadItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if(index.column()==2)
    {
        QStyleOptionProgressBar progressBarOption;
        progressBarOption.rect = option.rect.adjusted(0,1,0,-1);
        progressBarOption.fontMetrics = QApplication::fontMetrics();
        progressBarOption.minimum = 0;
        progressBarOption.maximum = 100;
        qint64 totalLength=index.data(DownloadModel::DataRole::TotalLengthRole).toLongLong(),
            completedLength=index.data(DownloadModel::DataRole::CompletedLengthRole).toLongLong();
        double val=qBound<double>(0,(double)completedLength/totalLength*100,100);
        progressBarOption.progress=val;
        progressBarOption.textAlignment = Qt::AlignCenter;
        progressBarOption.textVisible = true;
        if(totalLength>0)
            progressBarOption.text =QString("%1%").arg(val,0,'g',3);
        else
            progressBarOption.text =QString();
        QProgressBar progressBar;
        QApplication::style()->drawControl(QStyle::CE_ProgressBar, &progressBarOption, painter, &progressBar);
    }
    else
    {
        QStyledItemDelegate::paint(painter, option, index);
    }
}


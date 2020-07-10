#include "downloaditemdelegate.h"
#include <QApplication>
#include <QProgressBar>
#include <QPainter>
#include "downloadmodel.h"
DownloadItemDelegate::DownloadItemDelegate(QObject *parent) : QStyledItemDelegate(parent)
{

}

void DownloadItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    painter->save();
	QStyleOptionViewItem viewOption(option);
	initStyleOption(&viewOption, index);
	if (option.state.testFlag(QStyle::State_HasFocus))
		viewOption.state = viewOption.state ^ QStyle::State_HasFocus;

	QStyledItemDelegate::paint(painter, viewOption, index);
    if(index.column()==2)
    {
        QStyleOptionProgressBar progressBarOption;
		progressBarOption.rect = viewOption.rect.adjusted(0,1,0,-1);
        progressBarOption.fontMetrics = painter->fontMetrics();
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
    painter->restore();
}


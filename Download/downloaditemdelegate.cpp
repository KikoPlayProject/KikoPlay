#include "downloaditemdelegate.h"
#include <QApplication>
#include <QProgressBar>
#include <QPainter>
#include <QToolTip>
#include "downloadmodel.h"
#include "globalobjects.h"

namespace
{
static QString getTimeLeft(const DownloadTask *task)
{
    if (task->downloadSpeed == 0) return "--";
    qint64 leftLength(task->totalLength-task->completedLength);
    int totalSecond(ceil((double)leftLength/task->downloadSpeed));
    if (totalSecond>24*60*60) return ">24h";
    int minuteLeft(totalSecond/60);
    int secondLeft = totalSecond-minuteLeft*60;
    if (minuteLeft > 60)
    {
        int hourLeft=minuteLeft/60;
        minuteLeft=minuteLeft-hourLeft*60;
        return QString("%1:%2:%3").arg(hourLeft,2,10,QChar('0')).arg(minuteLeft,2,10,QChar('0')).arg(secondLeft,2,10,QChar('0'));
    }
    else
    {
        return QString("%1:%2").arg(minuteLeft,2,10,QChar('0')).arg(secondLeft,2,10,QChar('0'));
    }
}
}

DownloadItemDelegate::DownloadItemDelegate(QObject *parent) : QStyledItemDelegate(parent)
{
    itemHeight = marginTop;
    itemHeight += QFontMetrics(QFont(GlobalObjects::normalFont, titleFontSize)).height();   // title
    itemHeight += textSpacing;
    itemHeight += QFontMetrics(QFont(GlobalObjects::normalFont, statusFontSize)).height();  // status
    itemHeight += textSpacing;
    itemHeight += progressHeight;  // progress
    itemHeight += marginBottom;

    downloadProgress = new QProgressBar;
    downloadProgress->setObjectName(QStringLiteral("DownloadItemProgress"));
}

DownloadItemDelegate::~DownloadItemDelegate()
{
    if (downloadProgress) downloadProgress->deleteLater();
}

void DownloadItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem viewOption(option);
    initStyleOption(&viewOption, index);
    if (option.state.testFlag(QStyle::State_HasFocus))
        viewOption.state = viewOption.state ^ QStyle::State_HasFocus;

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHint(QPainter::SmoothPixmapTransform, true);


    QRect itemRect = viewOption.rect;
    itemRect.adjust(1, 1, -1, -1);
    painter->setBrush(QColor(255, 255, 255, 40));
    painter->setPen(Qt::NoPen);
    if (viewOption.state & QStyle::State_Selected)
    {
        painter->drawRoundedRect(itemRect, 8, 8);
    }
    else
    {
        if (viewOption.state & QStyle::State_MouseOver)
        {
            painter->drawRoundedRect(itemRect, 8, 8);
        }
    }

    const DownloadTask *task = (DownloadTask *)index.data(DownloadModel::DataRole::DownloadTaskRole).value<void *>();
    int y = marginTop + viewOption.rect.top();

    QRect textRect = viewOption.rect;
    textRect.setX(marginLeft);
    textRect.setY(y);
    static QFont textFont(GlobalObjects::normalFont);
    textFont.setPointSize(titleFontSize);
    painter->setFont(textFont);
    QFontMetrics fm = painter->fontMetrics();
    textRect.setHeight(fm.height());
    const int iconWidth = 32;
    textRect.setWidth(textRect.width() - iconWidth - marginLeft - marginRight - 8);
    QString title = task->title;
    if (fm.horizontalAdvance(title) > textRect.width())
    {
        title = fm.elidedText(title, Qt::ElideRight, textRect.width());
    }
    painter->setPen(QColor(240, 240, 240));
    painter->drawText(textRect, title);

    int iconBoxSize = qMin(fm.height(), 32);
    QRect iconRect(viewOption.rect.width() - marginRight - iconBoxSize - 8, y, iconWidth, iconBoxSize);
    index.siblingAtColumn(static_cast<int>(DownloadModel::Columns::STATUS)).data(Qt::DecorationRole).value<QIcon>().paint(painter, iconRect);

    y += textRect.height() + textSpacing;
    QString statusText{"%1/%2, %3 file(s)"};
    statusText = statusText.arg(formatSize(false, task->completedLength), formatSize(false, task->totalLength));
    statusText = statusText.arg(task->selectedIndexes.isEmpty() ? 1 : task->selectedIndexes.count(",") + 1);
    textFont.setPointSize(statusFontSize);
    painter->setFont(textFont);
    fm = painter->fontMetrics();
    QRect statusTextRect(marginLeft, y, fm.horizontalAdvance(statusText) + 2, fm.height());
    painter->setPen(QColor(190, 190, 190));
    painter->drawText(statusTextRect, statusText);

    QString speedText;
    if (task->status == DownloadTask::Downloading) speedText = formatSize(true, task->downloadSpeed);
    else speedText = index.siblingAtColumn(static_cast<int>(DownloadModel::Columns::STATUS)).data().toString();
    statusTextRect.setWidth(fm.horizontalAdvance(speedText) + 2);
    statusTextRect.moveRight(viewOption.rect.width() - marginRight);
    painter->drawText(statusTextRect, speedText);

    double progress = qBound<double>(0, (double)task->completedLength / task->totalLength * 100, 100);
    if (task->status == DownloadTask::Downloading && task->downloadSpeed > 0)
    {
        QString timeLeftText{"%2%, %1"};
        timeLeftText = timeLeftText.arg(getTimeLeft(task));
        timeLeftText = timeLeftText.arg(progress, 0, 'g', 3);
        const int timeLeftRight = viewOption.rect.width() - marginRight - 180;
        statusTextRect.setWidth(fm.horizontalAdvance(timeLeftText) + 2);
        statusTextRect.moveRight(timeLeftRight);
        painter->drawText(statusTextRect, timeLeftText);
    }

    y += statusTextRect.height() + textSpacing;

    if (task->status != DownloadTask::Complete && task->status != DownloadTask::Seeding)
    {
        QStyleOptionProgressBar progressBarOption;
        progressBarOption.rect = viewOption.rect.adjusted(marginLeft, y - viewOption.rect.top(), -marginRight, -marginBottom);
        progressBarOption.minimum = 0;
        progressBarOption.maximum = 100;
        progressBarOption.progress = progress;
        progressBarOption.textVisible = false;
        downloadProgress->style()->drawControl(QStyle::CE_ProgressBar, &progressBarOption, painter, downloadProgress);
    }
    painter->restore();
}

QSize DownloadItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    const DownloadTask *task = (DownloadTask *)index.data(DownloadModel::DataRole::DownloadTaskRole).value<void *>();
    QSize size = QStyledItemDelegate::sizeHint(option, index);
    int item_h = itemHeight;
    if (task->status == DownloadTask::Complete || task->status == DownloadTask::Seeding)
    {
        item_h -= (progressHeight + textSpacing);
    }
    size.setHeight(item_h);
    return size;
}


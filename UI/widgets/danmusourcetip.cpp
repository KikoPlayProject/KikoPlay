#include "danmusourcetip.h"
#include "Extension/Script/danmuscript.h"
#include "Extension/Script/scriptmanager.h"
#include "Play/Danmu/common.h"
#include "globalobjects.h"

DanmuSourceTip::DanmuSourceTip(const DanmuSource *sourceInfo, bool showCount, QWidget *parent) : QWidget{parent}
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setSource(sourceInfo, showCount);
}

void DanmuSourceTip::setSource(const DanmuSource *sourceInfo, bool showCount)
{
    QStringList fields;
    QString scriptTitle = sourceInfo->scriptId;
    auto curScript = GlobalObjects::scriptManager->getScript(sourceInfo->scriptId).dynamicCast<DanmuScript>();
    if (curScript)
    {
        scriptTitle = curScript->name();
        this->setToolTip(sourceInfo->scriptData);
    }
    else
    {
        if (sourceInfo->isKikoSource()) scriptTitle = "Kiko";
        else scriptTitle = tr("Local");
    }
    fields.append(scriptTitle);
    if (sourceInfo->duration > 0)
    {
        fields.append(sourceInfo->durationStr());
    }
    if (showCount)
    {
        fields.append(QString::number(sourceInfo->count));
    }

    this->text = fields.join('|');
    this->bgColor = curScript ? curScript->labelColor(): (sourceInfo->isKikoSource() ? QColor{ 19, 165, 200 } : QColor{ 43, 106, 176 });
    update();
}

void DanmuSourceTip::paintEvent(QPaintEvent *event)
{
    QPainter p(this);
    p.setRenderHints(QPainter::Antialiasing, true);
    p.setRenderHints(QPainter::TextAntialiasing, true);

    QPainterPath path;
    path.addRoundedRect(0, 0, width(), height(), 4, 4);
    p.setClipPath(path);
    p.fillRect(rect(), this->bgColor);

    p.setPen(Qt::white);
    p.drawText(rect(), Qt::AlignCenter, this->text);
}

void DanmuSourceTip::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        emit clicked();
    }
    QWidget::mouseReleaseEvent(event);
}

QSize DanmuSourceTip::sizeHint() const
{
    QFontMetrics fm(this->font());
    return fm.size(0, this->text) + QSize(8, 4);
}

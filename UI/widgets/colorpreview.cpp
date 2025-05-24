#include "colorpreview.h"
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QColorDialog>
#include "UI/stylemanager.h"

ColorPreview::ColorPreview(QWidget *parent) : QWidget(parent)
{
    setAttribute(Qt::WA_StyledBackground, true);
}

void ColorPreview::setColor(const QColor &c)
{
    useColor = true;
    color = c;
    update();
}

void ColorPreview::setPixmap(const QPixmap &p)
{
    useColor = false;
    pixmap = p;
    update();
}

void ColorPreview::setUseColorDialog(bool on)
{
    useColorDialog = on;
}

void ColorPreview::paintEvent(QPaintEvent *event)
{
    QPainter p(this);
    p.setRenderHints(QPainter::Antialiasing, true);
    p.setRenderHints(QPainter::SmoothPixmapTransform, true);
    QPainterPath path;
    path.addRoundedRect(0, 0, width(), height(), 4, 4);
    p.setClipPath(path);
    if (useColor)
    {
        p.fillRect(rect(), color);
    }
    else
    {
        QRect srcRect = pixmap.rect();
        float r = float(width()) / height();
        float sr = float(srcRect.width()) / srcRect.height();
        if (sr > r)
        {
            srcRect.setWidth(r * srcRect.height());
        }
        else
        {
            srcRect.setHeight(srcRect.width() / r);
        }
        srcRect.moveCenter(pixmap.rect().center());
        p.drawPixmap(rect(), pixmap, srcRect);
    }
    if (isEnter && isEnabled())
    {
        QPen pen;
        pen.setWidth(4);
        QColor borderColor = StyleManager::getStyleManager()->enableThemeColor() ? StyleManager::getStyleManager()->curThemeColor() : QColor{67, 156, 243};
        pen.setColor(borderColor);
        p.setPen(pen);

        p.drawRoundedRect(rect(), 4, 4);
    }
    if (!isEnabled())
    {
        p.fillRect(rect(), QColor(255, 255, 255, 50));
    }
    QWidget::paintEvent(event);
}

void ColorPreview::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        if (useColor && useColorDialog)
        {
            QColor c = QColorDialog::getColor(color, this, tr("Select Color"), QColorDialog::ColorDialogOption::ShowAlphaChannel);
            if (c != color && c.isValid())
            {
                color = c;
                update();
                emit colorChanged(color);
            }
        }
        emit click();
    }
    QWidget::mouseReleaseEvent(event);
}

void ColorPreview::enterEvent(QEnterEvent *event)
{
    isEnter = true;
    update();
    QWidget::enterEvent(event);
}

void ColorPreview::leaveEvent(QEvent *event)
{
    isEnter = false;
    update();
    QWidget::leaveEvent(event);
}

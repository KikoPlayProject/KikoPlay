#include "animeitemdelegate.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QGraphicsDropShadowEffect>
#include <QApplication>
#include <QPainter>
#include <QPushButton>
#include <QMouseEvent>
#include <QToolTip>
#include "animeinfo.h"
#define AnimeRole Qt::UserRole+1
namespace
{
int ItemWidth,ItemHeight;
class AnimeItemWidget : public QWidget
{
public:
    explicit AnimeItemWidget(QWidget *parent=nullptr):QWidget(parent),selected(false)
    {
        setObjectName(QStringLiteral("AnimeItemBg"));
        imgLabel = new QLabel(this);
        imgLabel->setAlignment(Qt::AlignCenter);
        imgLabel->setScaledContents(true);
        titleLabel=new QLabel(this);
        titleLabel->setAlignment(Qt::AlignCenter);
        titleLabel->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::Minimum);
        QVBoxLayout *aiVLayout=new QVBoxLayout(this);
        aiVLayout->addWidget(imgLabel);
        aiVLayout->addWidget(titleLabel);
        shadowEffect = new QGraphicsDropShadowEffect(this);
        shadowEffect->setOffset(0, 0);
        shadowEffect->setBlurRadius(12);
        imgLabel->setGraphicsEffect(shadowEffect);
        ItemWidth=130*logicalDpiX()/96;
        ItemHeight=185*logicalDpiY()/96;
        setFixedSize(ItemWidth,ItemHeight);
        titleLabel->setMaximumWidth(ItemWidth-20*logicalDpiX()/96);
    }
    void setTitle(const QString &title)
    {
        QFontMetrics fm(font());
        titleLabel->setText(fm.elidedText(title,Qt::ElideRight,ItemWidth-30*logicalDpiX()/96));
    }
    void setCover(const QPixmap &pixmap)
    {
        static QPixmap nullCover(":/res/images/cover.png");
        imgLabel->setPixmap(pixmap.isNull()?nullCover:pixmap);
    }
    void setHover(bool on)
    {
        if(!selected)
        {
            shadowEffect->setColor(on?Qt::blue:Qt::black);
            shadowEffect->setBlurRadius(12);
        }
    }
    void setSelected(bool on)
    {
        selected=on;
        if(selected)
        {
            shadowEffect->setColor(Qt::red);
            shadowEffect->setBlurRadius(14);
        }
    }
    QRect titleRect()
    {
        return titleLabel->geometry();
    }
private:
    QLabel *imgLabel,*titleLabel;
    QGraphicsDropShadowEffect *shadowEffect;
    bool selected;
};
}
AnimeItemDelegate::AnimeItemDelegate(QObject *parent):QStyledItemDelegate(parent),
    contentWidget(new AnimeItemWidget()),pixmap(ItemWidth,ItemHeight)
{

}

void AnimeItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem viewOption(option);
    initStyleOption(&viewOption,index);
    AnimeItemWidget *animeItemWidget=static_cast<AnimeItemWidget *>(contentWidget.data());
	const Anime *anime = (const Anime *)index.data(AnimeRole).value<void *>();
    animeItemWidget->setCover(anime->coverPixmap);
    animeItemWidget->setTitle(index.data(Qt::DisplayRole).toString());
    animeItemWidget->setSelected(viewOption.state.testFlag(QStyle::State_Selected));
    animeItemWidget->setHover(viewOption.state.testFlag(QStyle::State_MouseOver));

    pixmap.fill(Qt::transparent);
    contentWidget->render(&pixmap);
    painter->drawPixmap(option.rect.x(), option.rect.y(), pixmap);
}

bool AnimeItemDelegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    QMouseEvent *pEvent = static_cast<QMouseEvent *> (event);
    switch (event->type())
    {
    case QEvent::MouseMove:
    {
        AnimeItemWidget *animeItemWidget=static_cast<AnimeItemWidget *>(contentWidget.data());
        QRect rect=animeItemWidget->titleRect();
        rect.moveTo(rect.x() + option.rect.x(), rect.y() + option.rect.y());
        if(rect.contains(pEvent->pos()))
            QToolTip::showText(pEvent->globalPos(), index.model()->data(index,Qt::DisplayRole).toString());
        else
            QToolTip::hideText();
        break;
    }
    case QEvent::MouseButtonPress:
    {
        if(pEvent->button()==Qt::LeftButton && !(pEvent->modifiers()&Qt::ControlModifier)
                && !(pEvent->modifiers()&Qt::ShiftModifier))
            emit ItemClicked(index);
        break;
    }
    default:
        break;
    }
    return QStyledItemDelegate::editorEvent(event,model,option,index);
}

QSize AnimeItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    return QSize(ItemWidth,ItemHeight);
}

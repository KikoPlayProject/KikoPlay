#include "windowtip.h"
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QApplication>
//#include <QDesktopWidget>
#include <QScreen>
#include <QPropertyAnimation>
#include <QEvent>
#include <QPainter>
#include "Common/logger.h"
#include "globalobjects.h"

WindowTip::WindowTip(QWidget *parent)
    : QObject{parent}
{
    qRegisterMetaType<TipParams>("TipParams");
    parent->installEventFilter(this);
    QObject::connect(&hideTimer, &QTimer::timeout, this, [this](){
        bool changed = false;
        for (auto iter = tips.begin(); iter != tips.end();)
        {
            (*iter)->timeout -= timerInterval;
            if ((*iter)->timeout <= 0)
            {
                (*iter)->hide();
                unusedTips.append(*iter);
                iter = tips.erase(iter);
                changed = true;
            }
            else
            {
                ++iter;
            }
        }
        if(changed)
        {
            layoutTips();
        }
        if(tips.empty()) hideTimer.stop();
    });
}

void WindowTip::addTip(const TipParams &param)
{
    QWidget *parentWidget = static_cast<QWidget *>(this->parent());
    if (parentWidget->isHidden()) return;
    TipWindowWidget *tipWidget = nullptr;
    if (!param.group.isEmpty())
    {
        for (TipWindowWidget *tip : tips)
        {
            if (tip->tipParams.group == param.group)
            {
                tipWidget = tip;
                break;
            }
        }
        if (tipWidget)
        {
            tips.removeAll(tipWidget);
            tipWidget->reset(param);
        }
    }
    if (!tipWidget)
    {
        if (!unusedTips.empty())
        {
            tipWidget = unusedTips.takeLast();
            tipWidget->reset(param);
        }
        else
        {
            tipWidget = new TipWindowWidget(param, static_cast<QWidget *>(this->parent()));
            QObject::connect(tipWidget, &TipWindowWidget::closeClick, this, [this, tipWidget](){
                if (tips.removeAll(tipWidget) > 0)
                {
                    tipWidget->hide();
                    unusedTips.append(tipWidget);
                    layoutTips();
                }
            });
        }
    }
    tips.push_front(tipWidget);
    while (tips.size() > maxTipNum)
    {
        TipWindowWidget *w = tips.takeLast();
        w->hide();
        unusedTips.append(w);
    }
    Logger::logger()->log(Logger::APP, "[window_tip]" + param.message);
    layoutTips();
    if(!hideTimer.isActive()) hideTimer.start(timerInterval);
}

bool WindowTip::eventFilter(QObject *object, QEvent *event)
{
    QEvent::Type type = event->type();
    if (type == QEvent::Resize || type == QEvent::Move)
    {
        layoutTips();
    }
    return QObject::eventFilter(object, event);
}

void WindowTip::layoutTips()
{
    QWidget *parentWidget = static_cast<QWidget *>(this->parent());
    int totalHeight = top;
    for (auto tipWidget : tips)
    {
        bool justCreated = false;
        if (!tipWidget->isVisible())
        {
            justCreated = true;
            tipWidget->show();
            tipWidget->opacityAnimation->setStartValue(0.0f);
            tipWidget->opacityAnimation->setEndValue(1.0f);
            tipWidget->opacityAnimation->start();
        }

        int x = parentWidget->width() - tipWidget->width() - 4 * parentWidget->logicalDpiX() / 96;
        int y = totalHeight;
        QPoint widgetPos = parentWidget->mapToGlobal(QPoint(x, y));

        if (justCreated)
        {
            tipWidget->move(widgetPos);
        }
        else
        {
            QPropertyAnimation* positionAnimation = tipWidget->positionAnimation;
            positionAnimation->stop();
            positionAnimation->setStartValue(tipWidget->pos());
            positionAnimation->setEndValue(widgetPos);
            positionAnimation->start();
        }

        totalHeight += tipWidget->size().height() + 4 * parentWidget->logicalDpiX() / 96;
    }
}

TipWindowWidget::TipWindowWidget(const TipParams &param, QWidget *parent) : QWidget(parent), tipParams(param)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);

    timeout = param.timeout;
    titleLabel = new QLabel(param.title, this);
    titleLabel->setOpenExternalLinks(true);
    titleLabel->setFont(QFont(GlobalObjects::normalFont, 8));
    titleLabel->setObjectName(QStringLiteral("WindowTipTitleLabel"));
    titleLabel->setMaximumHeight(titleLabel->fontMetrics().height() * 2);

    messageLabel = new QLabel(param.message, this);
    messageLabel->setOpenExternalLinks(true);
    messageLabel->setWordWrap(true);
    messageLabel->setMaximumHeight(messageLabel->fontMetrics().height() * 4);
    messageLabel->setObjectName(QStringLiteral("WindowTipMsgLabel"));

    GlobalObjects::iconfont->setPointSize(titleLabel->font().pointSize() + 1);
    closeButton = new QPushButton(this);
    closeButton->setObjectName(QStringLiteral("TipCloseButton"));
    closeButton->setFont(*GlobalObjects::iconfont);
    closeButton->setText(QChar(0xe60b));
    closeButton->setVisible(param.showClose);
    QObject::connect(closeButton, &QPushButton::clicked, this, &TipWindowWidget::closeClick);

    QVBoxLayout *tipVLayout = new QVBoxLayout(this);
    QHBoxLayout *titleHLayout = new QHBoxLayout;
    titleHLayout->setContentsMargins(0, 0, 0, 0);
    titleHLayout->addWidget(titleLabel);
    titleHLayout->addStretch(1);
    titleHLayout->addWidget(closeButton);
    tipVLayout->addLayout(titleHLayout);
    tipVLayout->addSpacing(4 * logicalDpiY() / 96);
    tipVLayout->addWidget(messageLabel);
    tipVLayout->addStretch(1);

    //QDesktopWidget* desktop = QApplication::desktop();
    //QRect geometry = desktop->availableGeometry(parent);
    QRect geometry = QGuiApplication::primaryScreen()->availableGeometry();
    setFixedWidth(geometry.width() / 5);

    const int durationTimeMs = 120;
    positionAnimation = new QPropertyAnimation(this, "position", this);
    positionAnimation->setDuration(durationTimeMs);

    opacityAnimation = new QPropertyAnimation(this, "opacity", this);
    opacityAnimation->setDuration(durationTimeMs * 2);
}

void TipWindowWidget::reset(const TipParams &param)
{
    tipParams = param;
    if (!titleLabel || !messageLabel) return;
    titleLabel->setText(param.title);
    messageLabel->setText(param.message);
    timeout = param.timeout;
    closeButton->setVisible(param.showClose);
    update();
}

void TipWindowWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.fillRect(rect(), tipParams.bgColor);
}

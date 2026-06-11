#include "ElaComboBox.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QLayout>
#include <QListView>
#include <QMouseEvent>
#include <QLineEdit>
#include <QMetaObject>
#include <QPropertyAnimation>
#include <QVector>

#include "DeveloperComponents/ElaComboBoxStyle.h"
#include "ElaScrollBar.h"
#include "ElaTheme.h"
#include "UI/widgets/floatscrollbar.h"
#include "private/ElaComboBoxPrivate.h"
namespace
{
    constexpr int kDefaultComboItemHeight = 35;
    constexpr int kPopupExtraHeight = 8;

    int popupContentHeight(QAbstractItemView* view, int itemCount, int visibleItemCount)
    {
        if (!view || itemCount <= 0 || visibleItemCount <= 0)
        {
            return kDefaultComboItemHeight;
        }

        const int windowSize = qMin(itemCount, visibleItemCount);
        QVector<int> rowHeights;
        rowHeights.reserve(itemCount);
        for (int row = 0; row < itemCount; ++row)
        {
            rowHeights.append(qMax(view->sizeHintForRow(row), kDefaultComboItemHeight));
        }

        int maxWindowHeight = 0;
        for (int start = 0; start <= itemCount - windowSize; ++start)
        {
            int currentWindowHeight = 0;
            for (int offset = 0; offset < windowSize; ++offset)
            {
                currentWindowHeight += rowHeights.at(start + offset);
            }
            maxWindowHeight = qMax(maxWindowHeight, currentWindowHeight);
        }

        return maxWindowHeight > 0 ? maxWindowHeight : windowSize * kDefaultComboItemHeight;
    }
}

Q_PROPERTY_CREATE_Q_CPP(ElaComboBox, int, BorderRadius)
ElaComboBox::ElaComboBox(QWidget* parent)
    : QComboBox(parent), d_ptr(new ElaComboBoxPrivate())
{
    Q_D(ElaComboBox);
    d->q_ptr = this;
    d->_pBorderRadius = 3;
    d->_themeMode = eTheme->getThemeMode();
    setObjectName("ElaComboBox");
    setFixedHeight(35);
    d->_comboBoxStyle = new ElaComboBoxStyle(style());
    setStyle(d->_comboBoxStyle);

    //调用view 让container初始化
    setView(new QListView(this));
    QAbstractItemView* comboBoxView = this->view();
    comboBoxView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    comboBoxView->setTextElideMode(Qt::ElideRight);
    ElaScrollBar* scrollBar = new ElaScrollBar(this);
    comboBoxView->setVerticalScrollBar(scrollBar);
    //ElaScrollBar* floatVScrollBar = new ElaScrollBar(scrollBar, comboBoxView);
    new FloatScrollBar(scrollBar, comboBoxView);
    //floatVScrollBar->setIsAnimation(true);
    comboBoxView->setAutoScroll(false);
    comboBoxView->setSelectionMode(QAbstractItemView::NoSelection);
    comboBoxView->setObjectName("ElaComboBoxView");
    comboBoxView->setStyleSheet("#ElaComboBoxView{background-color:transparent;}");
    comboBoxView->setStyle(d->_comboBoxStyle);
    if (QListView* listView = qobject_cast<QListView*>(comboBoxView))
    {
        listView->setWordWrap(false);
        listView->setUniformItemSizes(false);
    }
    QWidget* container = this->findChild<QFrame*>();
    if (container)
    {
        container->setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
        container->setAttribute(Qt::WA_TranslucentBackground);
        container->setObjectName("ElaComboBoxContainer");
        container->setStyle(d->_comboBoxStyle);
        QLayout* layout = container->layout();
        while (layout->count())
        {
            layout->takeAt(0);
        }
        layout->addWidget(view());
        layout->setContentsMargins(6, 0, 6, 6);
#ifndef Q_OS_WIN
        container->setStyleSheet("background-color:transparent;");
#endif
    }
    QComboBox::setMaxVisibleItems(5);
    connect(eTheme, &ElaTheme::themeModeChanged, this, [=](ElaThemeType::ThemeMode themeMode) { d->_themeMode = themeMode; });
}

ElaComboBox::~ElaComboBox()
{
}

void ElaComboBox::setEditable(bool editable)
{
    Q_D(ElaComboBox);
    QComboBox::setEditable(editable);
    if (editable)
    {
        QColor fontColor = ElaThemeColor(d->_themeMode, BasicText);
        this->lineEdit()->setStyleSheet(QString("background-color:transparent; color:#%1;").arg(QString::number(fontColor.rgb(), 16)));
    }
}

void ElaComboBox::setWordWrap(bool on)
{
    Q_D(ElaComboBox);
    d->_comboBoxStyle->setWordWrap(on);
    view()->setTextElideMode(on ? Qt::ElideNone : Qt::ElideRight);
    if (QListView* listView = qobject_cast<QListView*>(view()))
    {
        listView->setWordWrap(on);
    }
}

void ElaComboBox::showPopup()
{
    Q_D(ElaComboBox);
    d->_comboBoxStyle->maxItemHeight = kDefaultComboItemHeight;
    bool oldAnimationEffects = qApp->isEffectEnabled(Qt::UI_AnimateCombo);
    qApp->setEffectEnabled(Qt::UI_AnimateCombo, false);
    QComboBox::showPopup();
    qApp->setEffectEnabled(Qt::UI_AnimateCombo, oldAnimationEffects);
    if (count() > 0)
    {
        QWidget* container = this->findChild<QFrame*>();
        if (container)
        {
            QMetaObject::invokeMethod(view(), "doItemsLayout", Qt::DirectConnection);
            int contentHeight = popupContentHeight(view(), count(), maxVisibleItems());
            int containerHeight = contentHeight + kPopupExtraHeight;
            view()->resize(view()->width(), contentHeight);
            const QModelIndex currentModelIndex = model()->index(currentIndex(), modelColumn(), rootModelIndex());
            if (currentModelIndex.isValid())
            {
                view()->scrollTo(currentModelIndex, QAbstractItemView::EnsureVisible);
            }
            container->move(container->x(), container->y() + 3);
            QLayout* layout = container->layout();
            while (layout->count())
            {
                layout->takeAt(0);
            }
            QPropertyAnimation* fixedSizeAnimation = new QPropertyAnimation(container, "maximumHeight");
            connect(fixedSizeAnimation, &QPropertyAnimation::valueChanged, this, [=](const QVariant& value) {
                container->setFixedHeight(value.toUInt());
            });
            fixedSizeAnimation->setStartValue(1);
            fixedSizeAnimation->setEndValue(containerHeight);
            fixedSizeAnimation->setEasingCurve(QEasingCurve::OutCubic);
            fixedSizeAnimation->setDuration(400);
            fixedSizeAnimation->start(QAbstractAnimation::DeleteWhenStopped);

            QPropertyAnimation* viewPosAnimation = new QPropertyAnimation(view(), "pos");
            connect(viewPosAnimation, &QPropertyAnimation::finished, this, [=]() {
                d->_isAllowHidePopup = true;
                layout->addWidget(view());
            });
            QPoint viewPos = view()->pos();
            viewPosAnimation->setStartValue(QPoint(viewPos.x(), viewPos.y() - view()->height()));
            viewPosAnimation->setEndValue(viewPos);
            viewPosAnimation->setEasingCurve(QEasingCurve::OutCubic);
            viewPosAnimation->setDuration(400);
            viewPosAnimation->start(QAbstractAnimation::DeleteWhenStopped);
        }
        //指示器动画
        QPropertyAnimation* rotateAnimation = new QPropertyAnimation(d->_comboBoxStyle, "pExpandIconRotate");
        connect(rotateAnimation, &QPropertyAnimation::valueChanged, this, [=](const QVariant& value) {
            update();
        });
        rotateAnimation->setDuration(300);
        rotateAnimation->setEasingCurve(QEasingCurve::InOutSine);
        rotateAnimation->setStartValue(d->_comboBoxStyle->getExpandIconRotate());
        rotateAnimation->setEndValue(-180);
        rotateAnimation->start(QAbstractAnimation::DeleteWhenStopped);
        QPropertyAnimation* markAnimation = new QPropertyAnimation(d->_comboBoxStyle, "pExpandMarkWidth");
        markAnimation->setDuration(300);
        markAnimation->setEasingCurve(QEasingCurve::InOutSine);
        markAnimation->setStartValue(d->_comboBoxStyle->getExpandMarkWidth());
        markAnimation->setEndValue(width() / 2 - d->_pBorderRadius - 6);
        markAnimation->start(QAbstractAnimation::DeleteWhenStopped);
    }
}

void ElaComboBox::hidePopup()
{
    Q_D(ElaComboBox);
    if (d->_isAllowHidePopup)
    {
        QWidget* container = this->findChild<QFrame*>();
        int containerHeight = container->height();
        if (container)
        {
            QLayout* layout = container->layout();
            while (layout->count())
            {
                layout->takeAt(0);
            }
            QPropertyAnimation* viewPosAnimation = new QPropertyAnimation(view(), "pos");
            connect(viewPosAnimation, &QPropertyAnimation::finished, this, [=]() {
                layout->addWidget(view());
                QMouseEvent focusEvent(QEvent::MouseButtonPress, QPoint(-1, -1), QPoint(-1, -1), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
                QApplication::sendEvent(parentWidget(), &focusEvent);
                QComboBox::hidePopup();
                container->setFixedHeight(containerHeight);
            });
            QPoint viewPos = view()->pos();
            connect(viewPosAnimation, &QPropertyAnimation::finished, this, [=]() { view()->move(viewPos); });
            viewPosAnimation->setStartValue(viewPos);
            viewPosAnimation->setEndValue(QPoint(viewPos.x(), viewPos.y() - view()->height()));
            viewPosAnimation->setEasingCurve(QEasingCurve::InCubic);
            viewPosAnimation->start(QAbstractAnimation::DeleteWhenStopped);

            QPropertyAnimation* fixedSizeAnimation = new QPropertyAnimation(container, "maximumHeight");
            connect(fixedSizeAnimation, &QPropertyAnimation::valueChanged, this, [=](const QVariant& value) {
                container->setFixedHeight(value.toUInt());
            });
            fixedSizeAnimation->setStartValue(container->height());
            fixedSizeAnimation->setEndValue(1);
            fixedSizeAnimation->setEasingCurve(QEasingCurve::InCubic);
            fixedSizeAnimation->start(QAbstractAnimation::DeleteWhenStopped);
            d->_isAllowHidePopup = false;
        }
        //指示器动画
        QPropertyAnimation* rotateAnimation = new QPropertyAnimation(d->_comboBoxStyle, "pExpandIconRotate");
        connect(rotateAnimation, &QPropertyAnimation::valueChanged, this, [=](const QVariant& value) {
            update();
        });
        rotateAnimation->setDuration(300);
        rotateAnimation->setEasingCurve(QEasingCurve::InOutSine);
        rotateAnimation->setStartValue(d->_comboBoxStyle->getExpandIconRotate());
        rotateAnimation->setEndValue(0);
        rotateAnimation->start(QAbstractAnimation::DeleteWhenStopped);
        QPropertyAnimation* markAnimation = new QPropertyAnimation(d->_comboBoxStyle, "pExpandMarkWidth");
        markAnimation->setDuration(300);
        markAnimation->setEasingCurve(QEasingCurve::InOutSine);
        markAnimation->setStartValue(d->_comboBoxStyle->getExpandMarkWidth());
        markAnimation->setEndValue(0);
        markAnimation->start(QAbstractAnimation::DeleteWhenStopped);
    }
}

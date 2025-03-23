#include "optionmenu.h"
#include <QVBoxLayout>
#include <QEvent>
#include <QMouseEvent>
#include <QFont>
#include <QPainter>
#include <QPropertyAnimation>
#include "UI/ela/Def.h"
#include "UI/ela/ElaIconButton.h"
#include <QTimer>
#include <QDebug>

OptionMenu::OptionMenu(QWidget *parent) : QStackedWidget{parent}
{
    _defaultPanel = new OptionMenuPanel(this);
    addWidget(_defaultPanel);
    setObjectName(QStringLiteral("OptionMenu"));
    setMouseTracking(true);

    const int durationTimeMs = 120;
    geoAnimation = new QPropertyAnimation(this, "geometry", this);
    geoAnimation->setDuration(durationTimeMs);
}

QSize OptionMenu::sizeHint() const
{
    return currentWidget()->sizeHint();
}

OptionMenuPanel *OptionMenu::addMenuPanel(const QString &name)
{
    OptionMenuPanel *menuPanel = new OptionMenuPanel(this);
    addWidget(menuPanel);
    if (!name.isEmpty())
    {
        panelMap.insert(name, menuPanel);
    }
    return menuPanel;
}

OptionMenuPanel *OptionMenu::getPanel(const QString &name)
{
    if (name == "default") return _defaultPanel;
    return panelMap.value(name, nullptr);
}

void OptionMenu::switchToPanel(OptionMenuPanel *menuPanel, SizeAnchor anchor)
{
    OptionMenuPanel *curPanel = static_cast<OptionMenuPanel *>(currentWidget());
    if (curPanel == menuPanel) return;
    panelStack.push_back(curPanel);

    updateSize(menuPanel->sizeHint(), anchor);
    setCurrentWidget(menuPanel);
}

void OptionMenu::back(SizeAnchor anchor)
{
    if (panelStack.empty()) return;
    OptionMenuPanel *backPanel = panelStack.back();
    panelStack.pop_back();

    updateSize(backPanel->sizeHint(), anchor);
    setCurrentWidget(backPanel);
}

void OptionMenu::setPanel(OptionMenuPanel *menuPanel, SizeAnchor anchor)
{
    OptionMenuPanel *curPanel = static_cast<OptionMenuPanel *>(currentWidget());
    if (curPanel == menuPanel) return;
    updateSize(menuPanel->sizeHint(), anchor);
    setCurrentWidget(menuPanel);
}

void OptionMenu::updateSize(const QSize &size, SizeAnchor anchor)
{
    QRect src(geometry());
    QRect dst(0, 0, size.width(), size.height());

    switch (anchor)
    {
    case SizeAnchor::BottomRight:
        dst.moveBottomRight(src.bottomRight());
        break;
    case SizeAnchor::BottomLeft:
        dst.moveBottomLeft(src.bottomLeft());
        break;
    case SizeAnchor::TopLeft:
        dst.moveTopLeft(src.topLeft());
        break;
    case SizeAnchor::TopRight:
        dst.moveTopRight(src.topRight());
        break;
    default:
        break;
    }

    geoAnimation->stop();
    geoAnimation->setStartValue(src);
    geoAnimation->setEndValue(dst);
    geoAnimation->start();
}

OptionMenuPanel::OptionMenuPanel(OptionMenu *menu) : QWidget{menu}, _menu(menu)
{
    setMouseTracking(true);
    QVBoxLayout *vLayout = new QVBoxLayout(this);
    vLayout->setContentsMargins(1, 0, 1, 0);
    vLayout->setSpacing(0);
    _menuLayout = new QVBoxLayout();
    _menuLayout->setContentsMargins(0, 0, 0, 0);
    _menuLayout->setSpacing(0);
    vLayout->addSpacing(10);
    vLayout->addLayout(_menuLayout);
    vLayout->addStretch(1);
    vLayout->addSpacing(10);
}

QSize OptionMenuPanel::sizeHint() const
{
    return layout()->sizeHint();
}

OptionMenuItem *OptionMenuPanel::addMenu(const QString &text)
{
    OptionMenuItem *menuItem = new OptionMenuItem(_menu, this);
    menuItem->setText(text);
    _menuLayout->addWidget(menuItem);
    menus.append(menuItem);
    if (_menu->currentWidget() == this && !_menu->isHidden())
    {
        QTimer::singleShot(0, [=](){
            _menu->updateSize(sizeHint());
        });
    }
    return menuItem;
}

OptionMenuItem *OptionMenuPanel::addTitleMenu(const QString &text)
{
    TitleMenuItem *titleMenuItem = new TitleMenuItem(_menu, this);
    titleMenuItem->setText(text);
    _menuLayout->addWidget(titleMenuItem);
    menus.append(titleMenuItem);
    return titleMenuItem;
}

void OptionMenuPanel::addSpliter()
{
    MenuSpliter *spliter = new MenuSpliter(this);
    _menuLayout->addWidget(spliter);
}

QVector<OptionMenuItem *> OptionMenuPanel::getCheckGroupMenus(const QString &group)
{
    QVector<OptionMenuItem *> groupMenus;
    std::copy_if(menus.begin(), menus.end(), std::back_inserter(groupMenus), [&](OptionMenuItem *item) { return item->_checkGroup == group; });
    return groupMenus;
}

void OptionMenuPanel::clearCheckGroup(const QString &group)
{
    for (OptionMenuItem *item : getCheckGroupMenus(group))
    {
        if (item->_isChecking)
        {
            item->_isChecking = false;
            item->update();
        }
    }
}

void OptionMenuPanel::removeMenu(OptionMenuItem *menu)
{
    menus.removeAll(menu);
    delete menu;
    if (_menu->currentWidget() == this && !_menu->isHidden())
    {
        QTimer::singleShot(0, [=](){
            _menu->updateSize(sizeHint());
        });
    }
}

void OptionMenuPanel::removeFlag(const QString &flag)
{
    if (flag.isEmpty()) return;
    bool hasRemoved = false;
    for (auto iter = menus.begin(); iter != menus.end();)
    {
        if ((*iter)->_flag == flag)
        {
            delete *iter;
            iter = menus.erase(iter);
            hasRemoved = true;
        }
        else
        {
            ++iter;
        }
    }
    if (hasRemoved)
    {
        if (_menu->currentWidget() == this && !_menu->isHidden())
        {
            QTimer::singleShot(0, [=](){
                _menu->updateSize(sizeHint());
            });
        }
    }
}

OptionMenuItem::OptionMenuItem(OptionMenu *menu, OptionMenuPanel *panel) : QWidget(panel), _menu(menu), _panel(panel)
{
    setFixHeight(40);
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    QFont menuFont = font();
    menuFont.setPointSize(12);
    setFont(menuFont);
}

QSize OptionMenuItem::sizeHint() const
{
    int minWidth = _contentMarginLeft + _contentMarginRight;
    if (_checkAble) minWidth += _iconWidth;
    if (_subMenuPanel && _showSubIndicator) minWidth += _iconWidth;
    if (!_text.isEmpty())
    {
        minWidth += qMin(fontMetrics().horizontalAdvance(_text), _maxTextWidth) + 10;
    }
    if (!_infoText.isEmpty()) minWidth += _maxInfoTextWidth;
    return QSize(qMax(_minWidth, minWidth), height());
}

void OptionMenuItem::setText(const QString &text)
{
    _text = text;
    update();
}

void OptionMenuItem::setInfoText(const QString &text)
{
    _infoText = text;
    update();
}

void OptionMenuItem::setChecking(bool check)
{
    if (!_checkAble) return;
    if (_checkGroup.isEmpty())
    {
        _isChecking = check;
    }
    else
    {
        if (!_isChecking && check)
        {
            for (OptionMenuItem *item : _panel->getCheckGroupMenus(_checkGroup))
            {
                if (item->isChecking())
                {
                    item->_isChecking = false;
                    item->update();
                }
            }
            _isChecking = true;
            update();
        }
    }
}

void OptionMenuItem::setWidget(QWidget *widget, bool full)
{
    if (!_menuHLayout)
    {
        _menuHLayout = new QHBoxLayout(this);
        int marginRight = _contentMarginRight;
        if (_subMenuPanel && _showSubIndicator)
        {
            marginRight += _iconWidth;
        }
        int marginLeft = _contentMarginLeft;
        if (_isChecking)
        {
            marginLeft += _iconWidth;
        }
        if (!_text.isEmpty())
        {
            marginLeft += qMin(fontMetrics().horizontalAdvance(_text), _maxTextWidth);
            marginLeft += 2;
        }
        _menuHLayout->setContentsMargins(marginLeft, 0, marginRight, 0);
    }
    if (!full) _menuHLayout->addStretch(1);
    _menuHLayout->addWidget(widget);
}

OptionMenuPanel *OptionMenuItem::addSubMenuPanel()
{
    if (_subMenuPanel) return _subMenuPanel;
    _subMenuPanel = _menu->addMenuPanel();
    return _subMenuPanel;
}

void OptionMenuItem::switchTo()
{
    if (_subMenuPanel)
    {
        _menu->switchToPanel(_subMenuPanel);
    }
}

bool OptionMenuItem::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::MouseButtonRelease:
    {
        QMouseEvent *e = static_cast<QMouseEvent*>(event);
        if (e->button() == Qt::LeftButton && _selectAble)
        {
            if (_checkAble)
            {
                setChecking(!_isChecking);
            }
            if (_subMenuPanel)
            {
                _menu->switchToPanel(_subMenuPanel);
            }
            emit click();
        }
        event->accept();
        return true;
    }
    case QEvent::Enter:
    case QEvent::Leave:
        update();
        break;
    default:
        break;
    }
    return QWidget::event(event);
}

void OptionMenuItem::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHints(QPainter::SmoothPixmapTransform | QPainter::Antialiasing | QPainter::TextAntialiasing);
    painter.setPen(Qt::NoPen);
    QRect paintRect = rect();
    if (underMouse() && _selectAble)
    {
        QColor hoverColor(150, 150, 150, 128);
        painter.setBrush(hoverColor);
    }
    else
    {
        painter.setBrush(Qt::transparent);
    }
    painter.drawRect(paintRect);
    painter.setPen(QColor(210, 210, 210));

    if (_isChecking)
    {
        QRect checkRect = paintRect;
        checkRect.setLeft(_contentMarginLeft - 4);
        checkRect.setRight(_contentMarginLeft - 4 + _iconWidth);
        painter.save();
        QFont iconFont = QFont("ElaAwesome");
        iconFont.setPointSize(12);
        painter.setFont(iconFont);
        painter.drawText(checkRect, Qt::AlignCenter, QChar((unsigned short)ElaIconType::Check));
        painter.restore();
    }

    if (!_text.isEmpty())
    {
        QRect textRect = paintRect;
        int left = _contentMarginLeft;
        int right = textRect.right() - (_textCenter ? _contentMarginLeft : _contentMarginRight);
        if (!_infoText.isEmpty()) right -= _maxInfoTextWidth;
        if (_checkAble) left += _iconWidth;
        if (_subMenuPanel && _showSubIndicator) right -= _iconWidth;
        textRect.setLeft(left);
        textRect.setRight(right);

        QString drawText = _text;
        if (painter.fontMetrics().horizontalAdvance(drawText) > _maxTextWidth)
        {
            drawText = painter.fontMetrics().elidedText(drawText, Qt::ElideMiddle, _maxTextWidth);
        }
        painter.drawText(textRect, Qt::AlignVCenter | (_textCenter ? Qt::AlignHCenter : 0), drawText);
    }

    if (!_infoText.isEmpty())
    {
        QRect infoTextRect = paintRect;
        int right = infoTextRect.right() - _contentMarginRight;
        if (_subMenuPanel && _showSubIndicator) right -= _iconWidth;
        infoTextRect.setLeft(right - _maxInfoTextWidth);
        infoTextRect.setRight(right);

        painter.save();
        QFont infoFont = font();
        infoFont.setPointSize(10);
        painter.setFont(infoFont);
        QString drawText = _infoText;
        if (painter.fontMetrics().horizontalAdvance(drawText) > _maxInfoTextWidth)
        {
            drawText = painter.fontMetrics().elidedText(_infoText, Qt::ElideRight, _maxInfoTextWidth);
        }
        painter.drawText(infoTextRect, Qt::AlignRight | Qt::AlignVCenter, drawText);
        painter.restore();
    }

    if (_subMenuPanel && _showSubIndicator)
    {
        QRect indicatorRect = paintRect;
        indicatorRect.setLeft(indicatorRect.right() - _contentMarginRight - _iconWidth);
        indicatorRect.setRight(indicatorRect.right() - _contentMarginRight);
        painter.save();
        QFont iconFont = QFont("ElaAwesome");
        iconFont.setPointSize(18);
        painter.setFont(iconFont);
        painter.drawText(indicatorRect, Qt::AlignCenter, QChar((unsigned short)ElaIconType::AngleRight));
        painter.restore();
    }

}

TitleMenuItem::TitleMenuItem(OptionMenu *menu, OptionMenuPanel *panel) : OptionMenuItem(menu, panel)
{
    _selectAble = false;
    _showSubIndicator = false;
    _menuHLayout = new QHBoxLayout(this);
    _menuHLayout->setContentsMargins(_contentMarginLeft/2, 0, _contentMarginRight, 0);
    _contentMarginLeft += _iconWidth;

    ElaIconButton *backBtn = new ElaIconButton(ElaIconType::AngleLeft, 24, 32, 32, this);
    backBtn->setLightHoverColor(Qt::transparent);
    backBtn->setDarkHoverColor(Qt::transparent);
    backBtn->setLightIconColor(QColor(210, 210, 210));
    backBtn->setLightHoverIconColor(Qt::white);
    backBtn->setDarkIconColor(QColor(210, 210, 210));
    backBtn->setDarkHoverIconColor(Qt::white);
    _menuHLayout->addWidget(backBtn);
    _menuHLayout->addStretch(1);
    QObject::connect(backBtn, &ElaIconButton::clicked, this, [=]() {
        _menu->back();
    });
}

MenuSpliter::MenuSpliter(OptionMenuPanel *panel) : QWidget(panel)
{
    setFixedHeight(3);
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
}

void MenuSpliter::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHints(QPainter::SmoothPixmapTransform | QPainter::Antialiasing | QPainter::TextAntialiasing);
    painter.setPen(Qt::NoPen);
    QRect paintRect = rect();
    paintRect.setTop(1);
    paintRect.setBottom(paintRect.bottom() - 1);
    painter.setBrush(QColor(100, 100, 100));
    painter.drawRect(paintRect);
}

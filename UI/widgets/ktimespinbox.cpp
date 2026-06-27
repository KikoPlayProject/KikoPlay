#include "ktimespinbox.h"
#include "UI/ela/ElaTheme.h"
#include "UI/ela/Def.h"
#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QListView>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QStandardItemModel>
#include <QWheelEvent>

namespace
{
    inline ElaThemeType::ThemeMode wheelThemeMode() { return eTheme->getThemeMode(); }

    // 按出现顺序返回各时间段在格式串中的种类序列（去重），用于确定滚轮列
    QString sectionOrder(const QString &fmt)
    {
        QString seen;
        for (QChar ch : fmt)
        {
            QChar c = ch.toLower();
            if ((c == 'h' || c == 'm' || c == 's') && !seen.contains(c))
                seen.append(c);
        }
        return seen;
    }
}

// ===========================================================================
// KTimeComboBoxStyle
// ===========================================================================
KTimeComboBoxStyle::KTimeComboBoxStyle(QStyle *style)
    : QProxyStyle(style)
{
    _themeMode = eTheme->getThemeMode();
    connect(eTheme, &ElaTheme::themeModeChanged, this, [this](ElaThemeType::ThemeMode themeMode) {
        _themeMode = themeMode;
    });
}

void KTimeComboBoxStyle::drawComplexControl(ComplexControl control, const QStyleOptionComplex *option,
                                            QPainter *painter, const QWidget *widget) const
{
    if (control == QStyle::CC_ComboBox)
    {
        if (const QStyleOptionComboBox *copt = qstyleoption_cast<const QStyleOptionComboBox *>(option))
        {
            painter->save();
            painter->setRenderHints(QPainter::SmoothPixmapTransform | QPainter::Antialiasing | QPainter::TextAntialiasing);
            bool isEnabled = copt->state.testFlag(QStyle::State_Enabled);
            bool isHover = copt->state.testFlag(QStyle::State_MouseOver);
            bool hasFocus = copt->state.testFlag(QStyle::State_HasFocus) || copt->state.testFlag(QStyle::State_On);

            painter->setPen(ElaThemeColor(_themeMode, BasicBorder));
            painter->setBrush(isEnabled ? (isHover ? ElaThemeColor(_themeMode, BasicHover)
                                                  : ElaThemeColor(_themeMode, BasicBase))
                                        : ElaThemeColor(_themeMode, BasicDisable));
            QRect comboBoxRect = copt->rect;
            comboBoxRect.adjust(_shadowBorderWidth, 1, -_shadowBorderWidth, -1);
            painter->drawRoundedRect(comboBoxRect, 3, 3);

            // 文字
            QRect textRect = subControlRect(QStyle::CC_ComboBox, copt, QStyle::SC_ScrollBarSubLine, widget);
            painter->setPen(isEnabled ? ElaThemeColor(_themeMode, BasicText)
                                      : ElaThemeColor(_themeMode, BasicTextDisable));
            painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, copt->currentText);

            // 底部展开标记线
            painter->setPen(Qt::NoPen);
            painter->setBrush(ElaThemeColor(_themeMode, PrimaryNormal));
            painter->drawRoundedRect(QRectF(comboBoxRect.center().x() - _pExpandMarkWidth,
                                            comboBoxRect.y() + comboBoxRect.height() - 3,
                                            _pExpandMarkWidth * 2, 3), 2, 2);

            // 右侧折叠箭头
            QRect expandIconRect = subControlRect(QStyle::CC_ComboBox, copt, QStyle::SC_ScrollBarAddPage, widget);
            if (expandIconRect.isValid())
            {
                QFont iconFont = QFont("ElaAwesome");
                iconFont.setPixelSize(17);
                painter->setFont(iconFont);
                painter->setPen(isEnabled ? (hasFocus ? ElaThemeColor(_themeMode, PrimaryNormal)
                                                      : ElaThemeColor(_themeMode, BasicText))
                                          : ElaThemeColor(_themeMode, BasicTextDisable));
                painter->translate(expandIconRect.x() + (qreal)expandIconRect.width() / 2,
                                   expandIconRect.y() + (qreal)expandIconRect.height() / 2);
                painter->rotate(_pExpandIconRotate);
                painter->translate(-expandIconRect.x() - (qreal)expandIconRect.width() / 2,
                                   -expandIconRect.y() - (qreal)expandIconRect.height() / 2);
                painter->drawText(expandIconRect, Qt::AlignCenter, QChar((unsigned short)ElaIconType::AngleDown));
            }
            painter->restore();
            return;
        }
    }
    QProxyStyle::drawComplexControl(control, option, painter, widget);
}

void KTimeComboBoxStyle::drawControl(ControlElement element, const QStyleOption *option,
                                     QPainter *painter, const QWidget *widget) const
{
    // 禁止 QComboBox 默认绘制文字标签：文字由 drawComplexControl 统一绘制，
    // 否则会被画两次造成错位重叠。
    if (element == QStyle::CE_ComboBoxLabel)
    {
        return;
    }
    // popup container 背景：阴影 + 圆角卡片
    if (element == QStyle::CE_ShapedFrame && widget && widget->objectName() == "KTimePopupContainer")
    {
        QRect viewRect = option->rect;
        painter->save();
        painter->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
        eTheme->drawEffectShadow(painter, viewRect, _shadowBorderWidth, 6);
        QRect fg(viewRect.x() + _shadowBorderWidth, viewRect.y(),
                 viewRect.width() - 2 * _shadowBorderWidth, viewRect.height() - _shadowBorderWidth);
        painter->setPen(ElaThemeColor(_themeMode, PopupBorder));
        painter->setBrush(ElaThemeColor(_themeMode, PopupBase));
        painter->drawRoundedRect(fg, 6, 6);
        painter->restore();
        return;
    }
    QProxyStyle::drawControl(element, option, painter, widget);
}

QRect KTimeComboBoxStyle::subControlRect(ComplexControl cc, const QStyleOptionComplex *opt,
                                         SubControl sc, const QWidget *widget) const
{
    if (cc == QStyle::CC_ComboBox)
    {
        if (const QStyleOptionComboBox *copt = qstyleoption_cast<const QStyleOptionComboBox *>(opt))
        {
            QRect comboRect = copt->rect;
            if (sc == QStyle::SC_ScrollBarSubLine)
            {
                // 文字区域：基于整个 combo 矩形，左边留内边距，右边给箭头留空间
                QRect textRect = comboRect;
                textRect.setLeft(_shadowBorderWidth + 12);
                textRect.setRight(comboRect.right() - _shadowBorderWidth - 28);
                return textRect;
            }
            if (sc == QStyle::SC_ScrollBarAddPage)
            {
                // 展开图标区域：右侧 24x24
                QRect expandIconRect(comboRect.right() - _shadowBorderWidth - 24,
                                     comboRect.y(), 24, comboRect.height());
                return expandIconRect;
            }
        }
    }
    return QProxyStyle::subControlRect(cc, opt, sc, widget);
}

QSize KTimeComboBoxStyle::sizeFromContents(ContentsType type, const QStyleOption *option,
                                           const QSize &size, const QWidget *widget) const
{
    if (type == QStyle::CT_ComboBox)
    {
        QSize comboBoxSize = QProxyStyle::sizeFromContents(type, option, size, widget);
        comboBoxSize.setWidth(comboBoxSize.width() + 26);
        return comboBoxSize;
    }
    return QProxyStyle::sizeFromContents(type, option, size, widget);
}

// ===========================================================================
// KTimeWheelColumn
// ===========================================================================
KTimeWheelColumn::KTimeWheelColumn(int maxValue, QWidget *parent)
    : QWidget(parent), m_maxValue(qMax(0, maxValue))
{
    setMouseTracking(true);
    setFixedHeight(170);
    setMinimumWidth(46);
    m_offset = contentYForIndex(m_value);
}

void KTimeWheelColumn::setMaxValue(int max)
{
    m_maxValue = qMax(0, max);
    if (m_value > m_maxValue) m_value = m_maxValue;
    animateTo(m_value);
}

void KTimeWheelColumn::setValue(int v)
{
    v = qBound(0, v, m_maxValue);
    if (m_value == v && qAbs(m_offset - contentYForIndex(v)) < 0.5) return;
    m_value = v;
    if (m_scrollAni) m_scrollAni->stop();
    m_offset = contentYForIndex(v);
    update();
}

void KTimeWheelColumn::stepBy(int delta)
{
    int target = qBound(0, m_value + delta, m_maxValue);
    animateTo(target);
}

void KTimeWheelColumn::setOffset(qreal o) { m_offset = o; update(); }

int KTimeWheelColumn::targetIndexForOffset(qreal offset) const
{
    return qBound(0, qRound(offset / (qreal)m_itemHeight), m_maxValue);
}

void KTimeWheelColumn::animateTo(int index)
{
    index = qBound(0, index, m_maxValue);
    if (m_scrollAni)
        m_scrollAni->stop();
    else
    {
        m_scrollAni = new QPropertyAnimation(this, "offset", this);
        m_scrollAni->setEasingCurve(QEasingCurve::OutCubic);
        m_scrollAni->setDuration(260);
    }
    int oldIndex = m_value;
    m_value = index;
    qreal fromOffset = m_offset;
    m_scrollAni->disconnect(this);
    connect(m_scrollAni, &QPropertyAnimation::finished, this, [this, oldIndex]() {
        m_offset = (qreal)contentYForIndex(m_value);
        update();
        if (oldIndex != m_value) emit valueChanged(m_value);
    });
    m_scrollAni->setStartValue(fromOffset);
    m_scrollAni->setEndValue((qreal)contentYForIndex(index));
    m_scrollAni->start();
}

void KTimeWheelColumn::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    ElaThemeType::ThemeMode tm = wheelThemeMode();
    int w = width();
    int h = height();
    int centerY = h / 2;

    QFont f = font();
    f.setPixelSize(16);
    painter.setFont(f);

    int centerIndex = qBound(0, qRound(m_offset / (qreal)m_itemHeight), m_maxValue);
    int firstVisible = qMax(0, centerIndex - (centerY / m_itemHeight) - 2);
    int lastVisible = qMin(m_maxValue, centerIndex + (centerY / m_itemHeight) + 2);

    QColor primary = ElaThemeColor(tm, PrimaryNormal);
    QColor baseText = ElaThemeColor(tm, BasicText);
    QColor popupBase = ElaThemeColor(tm, PopupBase);
    QColor borderHover = ElaThemeColor(tm, BasicBorderHover);

    for (int i = firstVisible; i <= lastVisible; ++i)
    {
        qreal itemCenterY = centerY - (m_offset - (qreal)i * m_itemHeight);
        if (itemCenterY < -m_itemHeight || itemCenterY > h + m_itemHeight) continue;
        QString text = QString::number(i).rightJustified(2, '0');
        qreal dist = qAbs(itemCenterY - centerY);
        qreal t = qBound(0.0, dist / ((qreal)(h / 2)), 1.0);
        int alpha = int(255 * (1.0 - t * 0.85));
        bool isCenter = (i == centerIndex);
        QColor c = isCenter ? primary : baseText;
        c.setAlpha(isCenter ? 255 : alpha);
        painter.setPen(c);
        QRect itemRect(0, (int)(itemCenterY - m_itemHeight / 2.0), w, m_itemHeight);
        painter.drawText(itemRect, Qt::AlignCenter, text);
    }

    // 选择带分隔线
    painter.setPen(QPen(borderHover, 1));
    painter.drawLine(0, centerY - m_itemHeight / 2, w, centerY - m_itemHeight / 2);
    painter.drawLine(0, centerY + m_itemHeight / 2, w, centerY + m_itemHeight / 2);

    // 上下渐隐遮罩
    QLinearGradient topGrad(0, 0, 0, centerY - m_itemHeight / 2);
    topGrad.setColorAt(0, popupBase);
    topGrad.setColorAt(1, QColor(popupBase.red(), popupBase.green(), popupBase.blue(), 0));
    painter.fillRect(QRect(0, 0, w, centerY - m_itemHeight / 2), topGrad);
    QLinearGradient botGrad(0, centerY + m_itemHeight / 2, 0, h);
    botGrad.setColorAt(0, QColor(popupBase.red(), popupBase.green(), popupBase.blue(), 0));
    botGrad.setColorAt(1, popupBase);
    painter.fillRect(QRect(0, centerY + m_itemHeight / 2, w, h - (centerY + m_itemHeight / 2)), botGrad);
}

void KTimeWheelColumn::wheelEvent(QWheelEvent *event)
{
    int delta = event->angleDelta().y();
    int steps = delta / 120;
    if (steps == 0) steps = delta > 0 ? 1 : -1;
    stepBy(-steps);
    event->accept();
}

void KTimeWheelColumn::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) return;
    m_dragging = true;
    m_pressY = event->pos().y();
    m_pressOffset = m_offset;
    if (m_scrollAni) m_scrollAni->stop();
    event->accept();
}

void KTimeWheelColumn::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_dragging) return;
    int dy = event->pos().y() - m_pressY;
    qreal newOffset = m_pressOffset - dy;
    newOffset = qBound(0.0, newOffset, (qreal)m_maxValue * m_itemHeight);
    m_offset = newOffset;
    update();
    event->accept();
}

void KTimeWheelColumn::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton || !m_dragging) return;
    m_dragging = false;
    int target = targetIndexForOffset(m_offset);
    animateTo(target);
    event->accept();
}

// ===========================================================================
// KTimeWheelView
// ===========================================================================
KTimeWheelView::KTimeWheelView(QWidget *parent) : QWidget(parent)
{
    setAutoFillBackground(false);
}

void KTimeWheelView::setSections(const QVector<int> &sectionMaxValues)
{
    m_sectionMax = sectionMaxValues;
    rebuildColumns();
}

void KTimeWheelView::paintEvent(QPaintEvent *)
{
    // 背景由 popup container (CE_ShapedFrame) 统一绘制；此处留空保持透明
}

void KTimeWheelView::rebuildColumns()
{
    qDeleteAll(m_columns);
    m_columns.clear();
    QHBoxLayout *layout = qobject_cast<QHBoxLayout *>(this->layout());
    if (!layout)
    {
        layout = new QHBoxLayout(this);
        layout->setContentsMargins(8, 0, 8, 0);
        layout->setSpacing(0);
    }
    while (layout->count()) delete layout->takeAt(0);

    for (int i = 0; i < m_sectionMax.size(); ++i)
    {
        auto *col = new KTimeWheelColumn(m_sectionMax[i], this);
        m_columns.append(col);
        connect(col, &KTimeWheelColumn::valueChanged, this, [this](int) {
            QTime t = compositeTime();
            emit timeChanged(t);
        });
        layout->addWidget(col, 1);
        if (i < m_sectionMax.size() - 1)
        {
            auto *sep = new QLabel(QStringLiteral(":"), this);
            sep->setObjectName(QStringLiteral("TimeWheelSep"));
            sep->setAlignment(Qt::AlignCenter);
            sep->setFixedWidth(14);
            layout->addWidget(sep);
        }
    }
}

QSize KTimeWheelView::sizeHint() const
{
    int cols = qMax(1, m_sectionMax.size());
    return QSize(46 * cols + 14 * (cols - 1) + 16, 170);
}

QTime KTimeWheelView::time() const
{
    return compositeTime();
}

void KTimeWheelView::setTime(const QTime &t)
{
    if (m_columns.isEmpty()) return;
    QVector<int> vals;
    if (m_sectionMax.size() == 3)
        vals << t.hour() << t.minute() << t.second();
    else if (m_sectionMax.size() == 2)
        vals << t.minute() << t.second();
    else if (m_sectionMax.size() == 1)
        vals << t.second();
    for (int i = 0; i < m_columns.size() && i < vals.size(); ++i)
    {
        m_columns[i]->blockSignals(true);
        m_columns[i]->setValue(vals[i]);
        m_columns[i]->blockSignals(false);
    }
}

void KTimeWheelView::setRange(const QTime &min, const QTime &max, const QVector<int> &sectionMaxValues)
{
    m_min = min;
    m_max = max;
    Q_UNUSED(sectionMaxValues);
}

QTime KTimeWheelView::compositeTime() const
{
    if (m_columns.isEmpty()) return QTime(0, 0, 0);
    if (m_sectionMax.size() == 3)
        return QTime(m_columns[0]->value(), m_columns[1]->value(), m_columns[2]->value());
    if (m_sectionMax.size() == 2)
        return QTime(0, m_columns[0]->value(), m_columns[1]->value());
    if (m_sectionMax.size() == 1)
        return QTime(0, 0, m_columns[0]->value());
    return QTime(0, 0, 0);
}

// ===========================================================================
// KTimeComboBox
// ===========================================================================
KTimeComboBox::KTimeComboBox(QWidget *parent) : QComboBox(parent)
{
    setObjectName("KTimeComboBox");
    setFixedHeight(35);
    m_style = new KTimeComboBoxStyle(style());
    setStyle(m_style);
    setModel(new QStandardItemModel(this));
    addItem(QString()); // 占位项，用于承载显示文本
    setEditable(false);
    refreshText();

    // 触发 QComboBox 创建内置 popup container，并配置为无边框透明弹出窗口
    setView(new QListView(this));
    view()->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    view()->setSelectionMode(QAbstractItemView::NoSelection);
    view()->setAutoScroll(false);
    view()->setObjectName("KTimeComboView");
    if (auto *lv = qobject_cast<QListView *>(view())) lv->setWordWrap(false);
    // 配置 container 为无边框透明弹出窗口（滚轮将放入其中，原 view 隐藏）
    if (QWidget *container = findChild<QFrame *>())
    {
        container->setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
        container->setAttribute(Qt::WA_TranslucentBackground);
        container->setObjectName("KTimePopupContainer");
    }

    connect(eTheme, &ElaTheme::themeModeChanged, this, [this](ElaThemeType::ThemeMode) { update(); });
}

KTimeComboBox::~KTimeComboBox()
{
}

QVector<int> KTimeComboBox::parseSections() const
{
    QVector<int> sections;
    QString seen = sectionOrder(m_format);
    for (QChar c : seen)
    {
        if (c == 'h') sections.append(23);
        else if (c == 'm') sections.append(59);
        else if (c == 's') sections.append(59);
    }
    return sections;
}

void KTimeComboBox::setDisplayFormat(const QString &format)
{
    m_format = format;
    // 尊重当前时间范围对小时段的需求：若范围跨度超过 1 小时而传入格式不含 h，
    // 自动升级为 hh:mm:ss，避免setDisplayFormat 覆盖 setTimeRange 的格式决策。
    bool needHour = m_maxTime.hour() > 0 || m_minTime.hour() > 0;
    if (needHour && !m_format.contains('h') && !m_format.contains('H'))
        m_format = QStringLiteral("hh:mm:ss");
    else if (!needHour && (m_format.contains('h') || m_format.contains('H')) &&
             m_maxTime != QTime(23, 59, 59)) // 默认范围未设置过，不强制降级
        m_format = QStringLiteral("mm:ss");
    refreshText();
}

void KTimeComboBox::setTimeRange(const QTime &min, const QTime &max)
{
    m_minTime = min;
    m_maxTime = max;
    // 根据范围跨度自动决定是否展示小时段：
    // 最大时间的小时数 > 0（跨度超过 1 小时）时，自动启用 hh:mm:ss 格式；
    // 否则保持 mm:ss。对外部传入的 setDisplayFormat 也做同样适配。
    if (m_maxTime.hour() > 0 || m_minTime.hour() > 0)
    {
        if (!m_format.contains('h') && !m_format.contains('H'))
            m_format = QStringLiteral("hh:mm:ss");
    }
    else
    {
        if (m_format.contains('h') || m_format.contains('H'))
            m_format = QStringLiteral("mm:ss");
    }
    if (m_time < m_minTime) setTime(m_minTime);
    else if (m_time > m_maxTime) setTime(m_maxTime);
    refreshText();
}

QTime KTimeComboBox::time() const
{
    return m_time;
}

void KTimeComboBox::setTime(const QTime &t)
{
    QTime clamped = t;
    if (clamped < m_minTime) clamped = m_minTime;
    if (clamped > m_maxTime) clamped = m_maxTime;
    m_time = clamped;
    refreshText();
}

void KTimeComboBox::refreshText()
{
    // 按段分组处理：连续的同类字符（h/m/s）合并为一段，只输出一次对应数值，
    // 段宽取该段字符数（如 "mm" -> 2 位）。其余字符原样输出。
    QString text;
    const QString &fmt = m_format;
    int i = 0;
    while (i < fmt.size())
    {
        QChar c = fmt.at(i);
        if (c == 'h' || c == 'H' || c == 'm' || c == 's')
        {
            int fieldWidth = 1;
            while (i + fieldWidth < fmt.size() && fmt.at(i + fieldWidth) == c)
                ++fieldWidth;
            int val = 0;
            if (c == 'h' || c == 'H') val = m_time.hour();
            else if (c == 'm') val = m_time.minute();
            else if (c == 's') val = m_time.second();
            text += QString::number(val).rightJustified(fieldWidth, '0');
            i += fieldWidth;
        }
        else
        {
            text += c;
            ++i;
        }
    }
    setItemText(0, text);
    setCurrentIndex(0);
    update();
}

void KTimeComboBox::showPopup()
{
    // 准备滚轮视图
    QVector<int> sections = parseSections();
    m_wheelView = new KTimeWheelView();
    m_wheelView->setSections(sections);
    m_wheelView->setRange(m_minTime, m_maxTime, sections);
    m_wheelView->setTime(m_time);
    connect(m_wheelView, &KTimeWheelView::timeChanged, this, [this](QTime t) {
        if (t < m_minTime) t = m_minTime;
        else if (t > m_maxTime) t = m_maxTime;
        QTime old = m_time;
        m_time = t;
        refreshText();
        if (t != old) emit userTimeChanged(t);
    });

    QWidget *container = findChild<QFrame *>();
    if (!container)
    {
        m_wheelView->deleteLater();
        m_wheelView = nullptr;
        QComboBox::showPopup();
        return;
    }

    // 隐藏 QComboBox 默认的列表 view，把滚轮放入 container
    view()->setVisible(false);
    QLayout *cl = container->layout();
    if (cl)
    {
        cl->setContentsMargins(4, 0, 4, 4);
        cl->addWidget(m_wheelView);
    }

    // 关闭 QComboBox 默认的滑入动画
    bool oldAnim = qApp->isEffectEnabled(Qt::UI_AnimateCombo);
    qApp->setEffectEnabled(Qt::UI_AnimateCombo, false);
    QComboBox::showPopup();
    qApp->setEffectEnabled(Qt::UI_AnimateCombo, oldAnim);

    // showPopup 后再设定 container 尺寸（滚轮尺寸），随后做展开动画
    QSize wheelSz = m_wheelView->sizeHint();
    if (wheelSz.isEmpty()) wheelSz = QSize(120, 170);
    int targetH = wheelSz.height() + 8;
    int targetW = wheelSz.width() + 8;
    m_wheelView->resize(wheelSz);
    container->setFixedSize(targetW, targetH);

    // 展开动画：container 高度从 1 增长到目标
    container->setFixedHeight(1);
    auto *hAni = new QPropertyAnimation(container, "maximumHeight", this);
    connect(hAni, &QPropertyAnimation::valueChanged, this, [container](const QVariant &v) {
        container->setFixedHeight(v.toUInt());
    });
    hAni->setStartValue(1);
    hAni->setEndValue(targetH);
    hAni->setEasingCurve(QEasingCurve::OutCubic);
    hAni->setDuration(300);
    hAni->start(QAbstractAnimation::DeleteWhenStopped);

    // 视图上滑入场
    if (m_wheelView)
    {
        QPoint targetPos = m_wheelView->pos();
        auto *posAni = new QPropertyAnimation(m_wheelView, "pos", this);
        posAni->setStartValue(QPoint(targetPos.x(), targetPos.y() - m_wheelView->height()));
        posAni->setEndValue(targetPos);
        posAni->setEasingCurve(QEasingCurve::OutCubic);
        posAni->setDuration(300);
        posAni->start(QAbstractAnimation::DeleteWhenStopped);
    }

    // 指示器动画
    auto *rotAni = new QPropertyAnimation(m_style, "ExpandIconRotate", this);
    connect(rotAni, &QPropertyAnimation::valueChanged, this, [this](const QVariant &) { update(); });
    rotAni->setStartValue(m_style->getExpandIconRotate());
    rotAni->setEndValue(-180.0);
    rotAni->setEasingCurve(QEasingCurve::InOutSine);
    rotAni->setDuration(300);
    rotAni->start(QAbstractAnimation::DeleteWhenStopped);

    constexpr int kShadowBorder = 4;
    auto *markAni = new QPropertyAnimation(m_style, "ExpandMarkWidth", this);
    markAni->setDuration(300);
    markAni->setEasingCurve(QEasingCurve::InOutSine);
    markAni->setStartValue(m_style->getExpandMarkWidth());
    markAni->setEndValue((qreal)(width() / 2 - kShadowBorder - 6));
    markAni->start(QAbstractAnimation::DeleteWhenStopped);
}

void KTimeComboBox::hidePopup()
{
    QWidget *container = findChild<QFrame *>();
    if (!container)
    {
        QComboBox::hidePopup();
        return;
    }
    // 收起动画进行中则直接走基类，避免重入创建多套动画
    if (m_closing)
    {
        QComboBox::hidePopup();
        return;
    }
    m_closing = true;

    // 指示器还原动画
    auto *rotAni = new QPropertyAnimation(m_style, "ExpandIconRotate", this);
    connect(rotAni, &QPropertyAnimation::valueChanged, this, [this](const QVariant &) { update(); });
    rotAni->setStartValue(m_style->getExpandIconRotate());
    rotAni->setEndValue(0.0);
    rotAni->setEasingCurve(QEasingCurve::InOutSine);
    rotAni->setDuration(200);
    rotAni->start(QAbstractAnimation::DeleteWhenStopped);

    auto *markAni = new QPropertyAnimation(m_style, "ExpandMarkWidth", this);
    markAni->setDuration(200);
    markAni->setEasingCurve(QEasingCurve::InOutSine);
    markAni->setStartValue(m_style->getExpandMarkWidth());
    markAni->setEndValue(0.0);
    markAni->start(QAbstractAnimation::DeleteWhenStopped);

    // 收起动画后再真正隐藏
    int startH = container->height();
    auto *hAni = new QPropertyAnimation(container, "maximumHeight", this);
    connect(hAni, &QPropertyAnimation::valueChanged, this, [container](const QVariant &v) {
        container->setFixedHeight(v.toUInt());
    });
    connect(hAni, &QPropertyAnimation::finished, this, [this]() {
        if (m_wheelView)
        {
            m_wheelView->setParent(nullptr);
            m_wheelView->deleteLater();
            m_wheelView = nullptr;
        }
        m_closing = false;
        QComboBox::hidePopup();
    });
    hAni->setStartValue(startH);
    hAni->setEndValue(1);
    hAni->setEasingCurve(QEasingCurve::InCubic);
    hAni->setDuration(150);
    hAni->start(QAbstractAnimation::DeleteWhenStopped);
}

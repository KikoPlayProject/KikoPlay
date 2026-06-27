#ifndef KTIMESPINBOX_H
#define KTIMESPINBOX_H

#include <QComboBox>
#include <QProxyStyle>
#include <QTime>
#include "UI/ela/ElaTheme.h"

class KTimeWheelView;
class QPropertyAnimation;

// ElaComboBox 风格的下拉时间选择框主框样式：圆角底框 + 底部展开标记线 + 右侧折叠箭头
class KTimeComboBoxStyle : public QProxyStyle
{
    Q_OBJECT
    Q_PROPERTY(qreal ExpandIconRotate READ getExpandIconRotate WRITE setExpandIconRotate)
    Q_PROPERTY(qreal ExpandMarkWidth READ getExpandMarkWidth WRITE setExpandMarkWidth)
public:
    explicit KTimeComboBoxStyle(QStyle *style = nullptr);
    void drawComplexControl(ComplexControl control, const QStyleOptionComplex *option,
                            QPainter *painter, const QWidget *widget = nullptr) const override;
    void drawControl(ControlElement element, const QStyleOption *option,
                     QPainter *painter, const QWidget *widget = nullptr) const override;
    QRect subControlRect(ComplexControl cc, const QStyleOptionComplex *opt,
                         SubControl sc, const QWidget *widget) const override;
    QSize sizeFromContents(ContentsType type, const QStyleOption *option,
                           const QSize &size, const QWidget *widget) const override;

    qreal getExpandIconRotate() const { return _pExpandIconRotate; }
    void setExpandIconRotate(qreal v) { _pExpandIconRotate = v; }
    qreal getExpandMarkWidth() const { return _pExpandMarkWidth; }
    void setExpandMarkWidth(qreal v) { _pExpandMarkWidth = v; }

private:
    qreal _pExpandIconRotate{0};
    qreal _pExpandMarkWidth{0};
    int _shadowBorderWidth{4};
    ElaThemeType::ThemeMode _themeMode;
};

// 单列滚轮：垂直列表，居中项高亮，上下渐隐遮罩
class KTimeWheelColumn : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal offset READ offset WRITE setOffset)
public:
    explicit KTimeWheelColumn(int maxValue, QWidget *parent = nullptr);
    void setValue(int v);
    int value() const { return m_value; }
    void setMaxValue(int max);
    void stepBy(int delta);
    qreal offset() const { return m_offset; }
    void setOffset(qreal o);

signals:
    void valueChanged(int value);

protected:
    void paintEvent(QPaintEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    int m_maxValue;          // 合法值上界（含），例如分 0..59
    int m_value{0};
    int m_itemHeight{34};
    qreal m_offset{0.0};     // 滚动偏移（以像素计，向下为正）
    int m_pressY{0};
    qreal m_pressOffset{0.0};
    bool m_dragging{false};
    QPropertyAnimation *m_scrollAni{nullptr};

    int targetIndexForOffset(qreal offset) const;
    void animateTo(int index);
    int contentYForIndex(int index) const { return index * m_itemHeight; }
};

// 多列滚轮容器：按显示格式排列各列（时/分/秒），用 ":" 分隔
class KTimeWheelView : public QWidget
{
    Q_OBJECT
public:
    explicit KTimeWheelView(QWidget *parent = nullptr);
    void setSections(const QVector<int> &sectionMaxValues); // 例如 {59,59} -> 分:秒
    QTime time() const;
    void setTime(const QTime &t);
    void setRange(const QTime &min, const QTime &max, const QVector<int> &sectionMaxValues);
    QSize sizeHint() const override;

signals:
    void timeChanged(const QTime &time);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QVector<KTimeWheelColumn *> m_columns;
    QTime m_min, m_max;
    QVector<int> m_sectionMax;
    void rebuildColumns();
    QTime compositeTime() const;
};

// 下拉时间选择框：ElaComboBox 风格外观 + 多列滚轮弹出
class KTimeComboBox : public QComboBox
{
    Q_OBJECT
public:
    explicit KTimeComboBox(QWidget *parent = nullptr);
    ~KTimeComboBox();

    void setDisplayFormat(const QString &format);
    QString displayFormat() const { return m_format; }
    void setTimeRange(const QTime &min, const QTime &max);
    QTime minimumTime() const { return m_minTime; }
    QTime maximumTime() const { return m_maxTime; }

    QTime time() const;
    void setTime(const QTime &t);

signals:
    // 仅用户通过弹窗选择改变时发出（语义同 QTimeEdit::userTimeChanged）
    void userTimeChanged(const QTime &time);

protected:
    void showPopup() override;
    void hidePopup() override;

private:
    void refreshText();
    QVector<int> parseSections() const;   // 按 m_format 返回各段上界

    QString m_format{"mm:ss"};
    QTime m_minTime{0, 0, 0};
    QTime m_maxTime{23, 59, 59};
    QTime m_time{0, 0, 0};
    KTimeComboBoxStyle *m_style{nullptr};
    KTimeWheelView *m_wheelView{nullptr};
    bool m_closing{false};                  // 收起动画进行中，防止 hidePopup 重入
};

#endif // KTIMESPINBOX_H

#ifndef KTAGPANEL_H
#define KTAGPANEL_H
#include <QWidget>


class KTagPanel : public QWidget
{
    Q_OBJECT
public:
    struct TagItem
    {
        QString text;
        QColor bgColor;
        QColor textColor;
        QIcon icon;
        QString tooltip;
        int flag;

        QRect rect;
        bool isHovered = false;

        void setIconFromSvgStr(const QString &svgStr, const QSize& renderSize = QSize(64, 64));
    };

    explicit KTagPanel(QWidget *parent = nullptr, int fontSize = 9);

    TagItem &addTag(const QString& text, const QString& tooltip = "",
                const QIcon& icon = QIcon(),
                const QColor& bgColor = QColor(0xf0f0f0),
                const QColor& textColor = QColor(Qt::white));
    TagItem &addTag(const TagItem &item);

    void clearTags();

    const TagItem &tag(int index) { return m_tags[index]; }
    int tagNum() const { return m_tags.size(); }

public:
    bool hasHeightForWidth() const override { return true; }
    int heightForWidth(int w) const override { return doLayout(w, false).height(); }
    QSize sizeHint() const override { return doLayout(200, false); }
    QSize minimumSizeHint() const override { return doLayout(0, false); }

signals:
    void tagClicked(int index);

protected:
    bool event(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QSize doLayout(int targetWidth, bool applyRects) const;


    QVector<TagItem> m_tags;

    int m_paddingX = 8;
    int m_paddingY = 2;
    int m_spacing = 2;
    int m_vspacing = 2;
    int m_iconSpacing = 2;
    int m_radius = 4;
};

#endif // KTAGPANEL_H

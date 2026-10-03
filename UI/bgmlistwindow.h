#ifndef BGMLISTWINDOW_H
#define BGMLISTWINDOW_H

#include <QWidget>
#include <QTreeView>
class BgmList;
class QPushButton;
class QComboBox;
class QActionGroup;
class BgmTreeView : public QTreeView
{
    Q_OBJECT
    Q_PROPERTY(QColor hoverColor READ getHoverColor WRITE setHoverColor)
    Q_PROPERTY(QColor normColor READ getNormColor WRITE setNormColor)
public:
    explicit BgmTreeView(QWidget *parent = nullptr);
    void reset() override;
    void doItemsLayout() override;

    bool isBangumiButtonHovered(const QModelIndex &index) const {return hoveredBangumiIndex == index;}
    bool isBangumiButtonPressed(const QModelIndex &index) const {return pressedBangumiIndex == index;}

    QColor getHoverColor() const {return hoverColor;}
    void setHoverColor(const QColor& color)
    {
        hoverColor =  color;
        emit hoverColorChanged(hoverColor);
    }
    QColor getNormColor() const {return normColor;}
    void setNormColor(const QColor& color)
    {
        normColor = color;
        emit normColorChanged(normColor);
    }
signals:
    void hoverColorChanged(const QColor &color);
    void normColorChanged(const QColor &color);
    void bangumiClicked(const QString &bgmId);
protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    bool viewportEvent(QEvent *event) override;
    void scrollContentsBy(int dx, int dy) override;
private:
    QModelIndex bangumiIndexAt(const QPoint &pos) const;
    void updateBangumiHover(const QPoint &pos);
    QPersistentModelIndex hoveredBangumiIndex, pressedBangumiIndex;
    QString pressedBangumiId;
    bool bangumiPressActive{false};
    bool lastLeftPressWasBangumi{false};
    QColor hoverColor, normColor;
};

class BgmListWindow : public QWidget
{
    Q_OBJECT
public:
    explicit BgmListWindow(QWidget *parent = nullptr);
private:
    BgmTreeView *bgmListView{nullptr};
    QComboBox *seasonIdCombo{nullptr};
    BgmList *bgmList{nullptr};
    QList<QPushButton *> weekDayBtnList;
    QStringList btnTitles={tr("Sun"),tr("Mon"),tr("Tue"),tr("Wed"),tr("Thu"),tr("Fri"),tr("Sat"),tr("All")};
    QVector<QAction *> scriptActions;
    QActionGroup *scriptCheckGroup{nullptr};
    QMenu *scriptMenu{nullptr};

    int weekDay;
signals:
    void searchBgm(const QString &item);
public slots:

    // QWidget interface
protected:
    virtual void showEvent(QShowEvent *event);
    virtual void hideEvent(QHideEvent *event);
    virtual void resizeEvent(QResizeEvent *event);
};

#endif // BGMLISTWINDOW_H

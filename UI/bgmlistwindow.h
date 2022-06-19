#ifndef BGMLISTWINDOW_H
#define BGMLISTWINDOW_H

#include <QWidget>
#include <QTreeView>
class BgmList;
class QPushButton;
class QComboBox;
class BgmTreeView : public QTreeView
{
    Q_OBJECT
    Q_PROPERTY(QColor hoverColor READ getHoverColor WRITE setHoverColor)
    Q_PROPERTY(QColor normColor READ getNormColor WRITE setNormColor)
public:
    using QTreeView::QTreeView;

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
private:
    QColor hoverColor, normColor;
};

class BgmListWindow : public QWidget
{
    Q_OBJECT
public:
    explicit BgmListWindow(QWidget *parent = nullptr);
private:
    BgmTreeView *bgmListView;
    QComboBox *seasonIdCombo, *bgmListScriptCombo;
    BgmList *bgmList;
    QList<QPushButton *> weekDayBtnList;
    QStringList btnTitles={tr("Sun"),tr("Mon"),tr("Tue"),tr("Wed"),tr("Thu"),tr("Fri"),tr("Sat"),tr("All")};

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

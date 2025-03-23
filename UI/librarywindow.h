#ifndef LIBRARYWINDOW_H
#define LIBRARYWINDOW_H

#include <QWidget>
#include <QTreeView>
#include <QListView>
#include "Common/notifier.h"
#include "UI/widgets/klineedit.h"
class Anime;
class QListView;
class QToolButton;
class QPushButton;
class QActionGroup;
class QButtonGroup;
class QSplitter;
class AnimeModel;
class AnimeFilterProxyModel;
class LabelProxyModel;
class AnimeItemDelegate;
class AnimeDetailInfoPage;
class DialogTip;

class AnimeListView : public QListView
{
    Q_OBJECT
    Q_PROPERTY(QColor itemBorderColor READ getItemBorderColor WRITE setItemBorderColor)
public:
    using QListView::QListView;

    QColor getItemBorderColor() const {return itemBorderColor;}
    void setItemBorderColor(const QColor& color)
    {
        itemBorderColor =  color;
        emit itemBorderColorChanged(itemBorderColor);
    }

signals:
    void itemBorderColorChanged(QColor color);

protected:
    virtual void resizeEvent(QResizeEvent *event);

private:
    QColor itemBorderColor;
};

class LabelTreeView : public QTreeView
{
    Q_OBJECT
    Q_PROPERTY(QColor topLevelColor READ getTopLevelColor WRITE setTopLevelColor)
    Q_PROPERTY(QColor childLevelColor READ getChildLevelColor WRITE setChildLevelColor)
    Q_PROPERTY(QColor countFColor READ getCountFColor WRITE setCountFColor)
    Q_PROPERTY(QColor countBColor READ getCountBColor WRITE setCountBColor)
public:
    using QTreeView::QTreeView;

    QColor getTopLevelColor() const {return topLevelColor;}
    void setTopLevelColor(const QColor& color)
    {
        topLevelColor =  color;
        emit topLevelColorChanged(topLevelColor);
    }
    QColor getChildLevelColor() const {return childLevelColor;}
    void setChildLevelColor(const QColor& color)
    {
        childLevelColor = color;
        emit childLevelColorChanged(childLevelColor);
    }
    QColor getCountFColor() const {return countFColor;}
    void setCountFColor(const QColor& color)
    {
        countFColor = color;
        emit countFColorChanged(countFColor);
    }
    QColor getCountBColor() const {return countBColor;}
    void setCountBColor(const QColor& color)
    {
        countBColor = color;
        emit countBColorChanged(countBColor);
    }
signals:
    void topLevelColorChanged(const QColor &color);
    void childLevelColorChanged(const QColor &color);
    void countFColorChanged(const QColor &color);
    void countBColorChanged(const QColor &color);
private:
    QColor topLevelColor, childLevelColor, countFColor, countBColor;
};
class AnimeFilterBox : public KLineEdit
{
    Q_OBJECT
public:
    explicit AnimeFilterBox(QWidget *parent = nullptr);

signals:
    void filterChanged(int type,const QString &filterStr);
private:
    QActionGroup *filterTypeGroup;
};
class LibraryWindow : public QWidget, public NotifyInterface
{
    Q_OBJECT
public:
    explicit LibraryWindow(QWidget *parent = nullptr);
    void initUI();
    QLayout *initLibrarayBtns(QWidget *parent);
    void initAnimeView();
    void initLabelView();
    void beforeClose();

private:
    AnimeModel *animeModel;
    AnimeFilterProxyModel *proxyModel;
    LabelProxyModel *labelProxyModel;
    AnimeListView *animeListView;
    LabelTreeView *labelView;
    QSplitter *splitter;
    bool isMovedCollapse{false};
    DialogTip *dialogTip;
    AnimeDetailInfoPage *detailPage{nullptr};
    QPushButton *filterCheckBtn;
    bool animeViewing;
    void searchAddAnime(Anime *srcAnime = nullptr);
signals:
    void playFile(const QString &file);
    void switchBackground(const QPixmap &pixmap, bool setPixmap);
    void updateFilterCounter(int c);
public slots:
    // QWidget interface
protected:
    virtual void showEvent(QShowEvent *event);
    virtual void hideEvent(QHideEvent *event);
    virtual void keyPressEvent(QKeyEvent *event);

    // NotifyInterface interface
public:
    virtual void showMessage(const QString &content, int flag, const QVariant & = QVariant());
};

#endif // LIBRARYWINDOW_H

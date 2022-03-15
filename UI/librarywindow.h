#ifndef LIBRARYWINDOW_H
#define LIBRARYWINDOW_H

#include <QWidget>
#include <QLineEdit>
#include <QTreeView>
#include "Common/notifier.h"
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
class AnimeFilterBox : public QLineEdit
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
    void beforeClose();

private:
    AnimeModel *animeModel;
    AnimeFilterProxyModel *proxyModel;
    LabelProxyModel *labelProxyModel;
    QListView *animeListView;
    LabelTreeView *labelView;
    QSplitter *splitter;
    DialogTip *dialogTip;
    AnimeDetailInfoPage *detailPage;
    bool animeViewing;
    void searchAddAnime(Anime *srcAnime = nullptr);
signals:
    void playFile(const QString &file);
    void switchBackground(const QPixmap &pixmap, bool setPixmap);
public slots:
    // QWidget interface
protected:
    virtual void showEvent(QShowEvent *event);
    virtual void hideEvent(QHideEvent *event);

    // NotifyInterface interface
public:
    virtual void showMessage(const QString &content, int flag);
};

#endif // LIBRARYWINDOW_H

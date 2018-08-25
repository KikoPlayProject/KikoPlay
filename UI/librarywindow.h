#ifndef LIBRARYWINDOW_H
#define LIBRARYWINDOW_H

#include <QWidget>
#include <QLineEdit>
class QListView;
class QToolButton;
class QGraphicsBlurEffect;
class QActionGroup;
class QButtonGroup;
class AnimeFilterBox : public QLineEdit
{
    Q_OBJECT
public:
    explicit AnimeFilterBox(QWidget *parent = 0);

signals:
    void filterChanged(int type,const QString &filterStr);
private:
    QActionGroup *filterTypeGroup;
};
class LibraryWindow : public QWidget
{
    Q_OBJECT
public:
    explicit LibraryWindow(QWidget *parent = nullptr);
private:
    QToolButton *allAnime,*likedAnime,*threeMonth,*halfYear,*year;
    QListView *animeListView;
    QWidget *contentWidget;
    QGraphicsBlurEffect *blurEffect;
    QWidget *detailInfoPage;
    QButtonGroup *btnGroup;
    const QString btnText[4] = { tr("All"),tr("Recent Three Months"),tr("Recent Half Year"),tr("Recent Year") };
    void updateButtonText();
signals:
    void playFile(const QString &file);
public slots:
    void showDetailInfo(const QModelIndex &index);
    // QWidget interface
protected:
    virtual void showEvent(QShowEvent *event);
    virtual void hideEvent(QHideEvent *event);
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void resizeEvent(QResizeEvent *event);
};

#endif // LIBRARYWINDOW_H

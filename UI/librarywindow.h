#ifndef LIBRARYWINDOW_H
#define LIBRARYWINDOW_H

#include <QWidget>
#include <QLineEdit>
class QListView;
class QToolButton;
class QPushButton;
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
class LabelPanel : public QWidget
{
	Q_OBJECT
public:
    explicit LabelPanel(QWidget *parent = 0, bool allowDelete=false);
    void addTag(const QString &tag);
    void removeTag(const QString &tag);
signals:
    void tagStateChanged(const QString &tag, bool checked);
    void deleteTag(const QString &tag);
private:
    QMap<QString,QPushButton *> tagList;
    bool showDelete;
};
class LibraryWindow : public QWidget
{
    Q_OBJECT
public:
    explicit LibraryWindow(QWidget *parent = nullptr);
private:
    QToolButton *allAnime,*likedAnime,*threeMonth,*halfYear,*year;
    QListView *animeListView;
    QWidget *detailInfoPage;
    QPushButton *tagCollapseButton;
    QWidget *filterPage;
    LabelPanel *timePanel,*tagPanel;
    QButtonGroup *btnGroup;
signals:
    void playFile(const QString &file);
public slots:
    // QWidget interface
protected:
    virtual void showEvent(QShowEvent *event);
    virtual void hideEvent(QHideEvent *event);
    virtual void resizeEvent(QResizeEvent *event);
};

#endif // LIBRARYWINDOW_H

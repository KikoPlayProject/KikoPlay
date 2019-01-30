#ifndef LIBRARYWINDOW_H
#define LIBRARYWINDOW_H

#include <QWidget>
#include <QLineEdit>
class QListView;
class QTreeView;
class QToolButton;
class QPushButton;
class QGraphicsBlurEffect;
class QActionGroup;
class QButtonGroup;
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
class LibraryWindow : public QWidget
{
    Q_OBJECT
public:
    explicit LibraryWindow(QWidget *parent = nullptr);
private:
    QListView *animeListView;
    QTreeView *labelView;
    QPushButton *tagCollapseButton;
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

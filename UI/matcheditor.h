#ifndef MATCHEDITOR_H
#define MATCHEDITOR_H

#include "framelessdialog.h"

class PlayListItem;
struct MatchInfo;
class QStackedLayout;
class QToolButton;
class QLineEdit;
class QTreeWidget;
class MatchEditor : public CFramelessDialog
{
    Q_OBJECT
public:
    explicit MatchEditor(const PlayListItem *item, MatchInfo *matchInfo=nullptr, QWidget *parent = nullptr);
    ~MatchEditor();

    MatchInfo *getMatchInfo(){return matchInfo;}
private:
    MatchInfo *matchInfo;
    QToolButton *searchPage,*customPage;
    QStackedLayout *contentStackLayout;
    QLineEdit *keywordEdit,*animeEdit,*subtitleEdit;
    QTreeWidget *searchResult;
    int searchLocation;
    QPushButton *searchButton;

    QWidget *setupSearchPage(MatchInfo *matchInfo);
    QWidget *setupCustomPage();
signals:

public slots:
    void search();
    // QObject interface
public:
    virtual bool eventFilter(QObject *watched, QEvent *event);

    // CFramelessDialog interface
protected:
    virtual void onAccept();
    virtual void onClose();
};

#endif // MATCHEDITOR_H

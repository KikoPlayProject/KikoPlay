#ifndef ADDPOOL_H
#define ADDPOOL_H

#include "framelessdialog.h"
class QStackedLayout;
class QToolButton;
class QLineEdit;
class QTreeWidget;
class AddPool : public CFramelessDialog
{
    Q_OBJECT
public:
    explicit AddPool(QWidget *parent = nullptr, const QString &srcAnime="",const QString &srcEp="");
    QString animeTitle,epTitle;
private:
    QToolButton *searchPage,*customPage;
    QStackedLayout *contentStackLayout;
    QLineEdit *keywordEdit, *animeEdit,*epEdit;
    QTreeWidget *searchResult;
    int searchLocation;
    bool renamePool;

    QWidget *setupSearchPage();
    QWidget *setupCustomPage();

    // CFramelessDialog interface
protected:
    virtual void onAccept();
    virtual void onClose();
};

#endif // ADDPOOL_H

#ifndef MATCHEDITOR_H
#define MATCHEDITOR_H

#include "framelessdialog.h"
#include "MediaLibrary/animeinfo.h"

class PlayListItem;
class QToolButton;
class QLineEdit;
class MatchEditor;
class QComboBox;
class MatchEditor : public CFramelessDialog
{
    Q_OBJECT
public:
    MatchEditor(const PlayListItem *item, QList<const PlayListItem *> *batchItems, QWidget *parent = nullptr);

    QString anime;
    EpInfo singleEp;
    QList<EpInfo> epList;
    QList<bool> epCheckedList;

    QList<const PlayListItem *> *batchItems;
    const PlayListItem *curItem;

private:
    QToolButton *searchPage,*customPage;
    QLineEdit *animeEdit, *epEdit, *epIndexEdit;
    QComboBox *epTypeCombo;

    QAbstractItemModel *animeModel, *epModel;

    QWidget *setupSearchPage(const QString &srcAnime="");
    QWidget *setupCustomPage(const QString &srcAnime="", const EpInfo &ep=EpInfo());

    // CFramelessDialog interface
protected:
    virtual void onAccept();
    virtual void onClose();
};

#endif // MATCHEDITOR_H

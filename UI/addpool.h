#ifndef ADDPOOL_H
#define ADDPOOL_H

#include "framelessdialog.h"
#include "MediaLibrary/animeinfo.h"
class QStackedLayout;
class QToolButton;
class QLineEdit;
class QComboBox;
class QTreeView;
class AddPool : public CFramelessDialog
{
    Q_OBJECT
public:
    AddPool(QWidget *parent = nullptr, const QString &srcAnime="", const EpInfo &ep=EpInfo());
    QString anime, ep;
    EpType epType;
    double epIndex;
private:
    QLineEdit *animeEdit, *epEdit, *epIndexEdit;
    QComboBox *epTypeCombo;
    int pageIndex{0};

    QTreeView *epView;
    QAbstractItemModel *animeModel, *epModel;

    bool renamePool;

    QWidget *setupSearchPage(const QString &srcAnime="", const EpInfo &ep=EpInfo());
    QWidget *setupCustomPage(const QString &srcAnime="", const EpInfo &ep=EpInfo());

    // CFramelessDialog interface
protected:
    virtual void onAccept();
    virtual void onClose();
};

#endif // ADDPOOL_H

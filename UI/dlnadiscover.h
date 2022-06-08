#ifndef DLNADISCOVER_H
#define DLNADISCOVER_H
#include "framelessdialog.h"

struct PlayListItem;
class QTreeView;
class DLNADiscover : public CFramelessDialog
{
    Q_OBJECT
public:
    DLNADiscover(const PlayListItem *item, QWidget *parent = nullptr);
    QString deviceName;
private:
    std::function<bool(const QModelIndex &)> playItemFunc;
    QTreeView *deviceView;
    // CFramelessDialog interface
protected:
    virtual void onAccept();
};

#endif // DLNADISCOVER_H

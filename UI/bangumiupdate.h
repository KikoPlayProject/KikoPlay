#ifndef BANGUMIUPDATE_H
#define BANGUMIUPDATE_H
#include "framelessdialog.h"
struct Anime;
class BangumiUpdate : public CFramelessDialog
{
    Q_OBJECT
public:
    explicit BangumiUpdate(Anime *anime, QWidget *parent = nullptr);
};

#endif // BANGUMIUPDATE_H

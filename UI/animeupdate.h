#ifndef BANGUMIUPDATE_H
#define BANGUMIUPDATE_H
#include "framelessdialog.h"
class Anime;
class AnimeUpdate : public CFramelessDialog
{
    Q_OBJECT
public:
    explicit AnimeUpdate(Anime *anime, QWidget *parent = nullptr);
};

#endif // BANGUMIUPDATE_H

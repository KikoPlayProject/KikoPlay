#ifndef ANIMEDETAILINFO_H
#define ANIMEDETAILINFO_H

#include "framelessdialog.h"
struct Anime;
class AnimeDetailInfo : public CFramelessDialog
{
    Q_OBJECT
public:
    explicit AnimeDetailInfo(Anime *anime, QWidget *parent = nullptr);
private:
    Anime *currentAnime;
    QStringList epNames;

    QWidget *setupOverviewPage();
    QWidget *setupEpisodesPage();
    QWidget *setupCharacterPage();
    QWidget *setupTagPage();
signals:
    void playFile(const QString &file);
public slots:
};

#endif // ANIMEDETAILINFO_H

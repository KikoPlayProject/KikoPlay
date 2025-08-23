#ifndef ANIMESCRAPINGTASK_H
#define ANIMESCRAPINGTASK_H


#include "Common/taskpool.h"
#include "MediaLibrary/animeinfo.h"

class AnimeScrapingTask : public KTask
{
public:
    AnimeScrapingTask(Anime *anime, const MatchResult &match);

protected:
    virtual TaskStatus runTask() override;

private:
    Anime *curAnime;
    MatchResult curMatch;

};

#endif // ANIMESCRAPINGTASK_H

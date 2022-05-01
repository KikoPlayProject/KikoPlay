#ifndef ANIMEBATCHACTION_H
#define ANIMEBATCHACTION_H
#include "framelessdialog.h"

class AnimeModel;
class AnimeBatchAction : public CFramelessDialog
{
    Q_OBJECT
public:
    AnimeBatchAction(AnimeModel *animeModel, QWidget *parent = nullptr);
};

#endif // ANIMEBATCHACTION_H

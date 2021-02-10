#ifndef EPISODEEDITOR_H
#define EPISODEEDITOR_H

#include "framelessdialog.h"
class Anime;
class EpisodesModel;
class EpisodeEditor : public CFramelessDialog
{
    Q_OBJECT
public:
    explicit EpisodeEditor(Anime *anime,QWidget *parent = nullptr);
    bool episodeChanged();
private:
    QStringList epNames;
    EpisodesModel *epModel;
};

#endif // EPISODEEDITOR_H

#ifndef PLAYLISTITEM_H
#define PLAYLISTITEM_H
#include <QObject>

class PlayList;
class PlayListItem
{
public:
    PlayListItem(PlayListItem *p = nullptr, bool leaf = false, int insertPosition = -1);
    ~PlayListItem();

    bool hasPool() const;
    void setLevel(int newLevel);
    void moveTo(PlayListItem *newParent, int insertPosition = -1);

    static PlayList *playlist;

    PlayListItem *parent;
    QList<PlayListItem *> *children;

    enum PlayState
    {
        UNPLAY, UNFINISH, FINISH
    };
    enum Marker
    {
        M_RED, M_BLUE, M_GREEN, M_ORANGE, M_PINK, M_YELLOW, M_NONE
    };

    int playTime;
    PlayState playTimeState;
    int level;
    bool isBgmCollection;
    Marker marker;

    QString title;
    QString animeTitle;
    QString path, folderPath;
    QString poolID;
};

#endif // PLAYLISTITEM_H

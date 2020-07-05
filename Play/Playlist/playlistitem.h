#ifndef PLAYLISTITEM_H
#define PLAYLISTITEM_H
#include <QObject>

class PlayList;
class PlayListItem
{
public:
    PlayListItem(PlayListItem *p = nullptr, bool leaf = false, int insertPosition = -1);
    ~PlayListItem();

    void setLevel(int newLevel);
    void moveTo(PlayListItem *newParent, int insertPosition = -1);

    static PlayList *playlist;

    PlayListItem *parent;
    QList<PlayListItem *> *children;

    int playTime;
    int playTimeState;
    int level;
    bool isBgmCollection;

    QString title;
    QString animeTitle;
    QString path, folderPath;
    QString poolID;
};

#endif // PLAYLISTITEM_H

#ifndef ANIMEEPISODEEDITOR_H
#define ANIMEEPISODEEDITOR_H
#include "framelessdialog.h"
#include "MediaLibrary/animeinfo.h"

class QComboBox;
class AnimeEpisodeEditor : public CFramelessDialog
{
    Q_OBJECT
public:
    AnimeEpisodeEditor(Anime *anime, const EpInfo &ep, QWidget *parent = nullptr);

public:
    Anime *curAnime{nullptr};
    EpInfo curEp;
    bool epChanged{false};
private:
    const int EpRole = Qt::UserRole + 1;
    QVector<EpInfo> epList;
    void addParentItem(QComboBox *combo, const QString& text) const;
    void addChildItem(QComboBox *combo, const QString& text, int epListIndex) const;
};

#endif // ANIMEEPISODEEDITOR_H

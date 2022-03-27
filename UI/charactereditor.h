#ifndef CHARACTEREDITOR_H
#define CHARACTEREDITOR_H
#include "framelessdialog.h"
struct Character;
class Anime;
class CharacterEditor : public CFramelessDialog
{
    Q_OBJECT
public:
    CharacterEditor(Anime *anime, const Character *crt, QWidget *parent = nullptr);

    QString name, link, actor;
private:
    Anime *curAnime;
    const Character *curCrt;
    // CFramelessDialog interface
protected:
    virtual void onAccept();
};

#endif // CHARACTEREDITOR_H

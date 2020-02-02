#ifndef ANIMEDETAILINFO_H
#define ANIMEDETAILINFO_H

#include "framelessdialog.h"
#include <QMap>
struct Anime;
struct Character;
class CharacterItem : public QWidget
{
     Q_OBJECT
public:
    explicit CharacterItem(Character *character, QWidget *parent=nullptr);
    void refreshIcon();
    Character *crt;
private:
    QLabel *iconLabel;
protected:
    virtual void mousePressEvent(QMouseEvent *event);
signals:
    void updateCharacter(CharacterItem *crtItem);
};
class LabelPanel : public QWidget
{
    Q_OBJECT
public:
    explicit LabelPanel(QWidget *parent = nullptr, bool allowDelete=false);
    void addTag(const QStringList &tags);
    void removeTag();
    QStringList getSelectedTags();
signals:
    void tagStateChanged(const QString &tag, bool checked);
    void deleteTag(const QString &tag);
private:
    QMap<QString,QPushButton *> tagList;
    bool showDelete;
    QWidget *contentWidget;
};
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
    QWidget *setupCapturePage();
signals:
    void playFile(const QString &file);
public slots:
};

#endif // ANIMEDETAILINFO_H

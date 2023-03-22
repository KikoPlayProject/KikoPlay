#ifndef ANIMEDETAILINFOPAGE_H
#define ANIMEDETAILINFOPAGE_H

#include <QWidget>
#include <QMap>
#include "MediaLibrary/animeinfo.h"
class Anime;
class QLabel;
class QTextEdit;
class EpisodesModel;
class EpItemDelegate;
struct Character;
class QListWidget;
class QPushButton;
class QLineEdit;
class CaptureListModel;
class QAction;
class QMenu;
class QStackedLayout;
class CharacterListModel;
class CharacterItemDelegate;
class QListView;

class CharacterWidget : public QWidget
{
     Q_OBJECT
public:
    CharacterWidget(const Character &character, QWidget *parent=nullptr);
    void refreshIcon(const QByteArray &imgBytes = QByteArray());
    void refreshText();
    Character crt;
    static int maxCrtItemWidth, crtItemHeight;
private:
    QLabel *iconLabel, *nameLabel, *infoLabel;
    constexpr static const int iconSize = 60;
protected:
    virtual void mouseDoubleClickEvent(QMouseEvent *) override;
signals:
    void updateCharacter(CharacterWidget *crtItem);
    void selectLocalImage(CharacterWidget *crtItem);
    void pasteImage(CharacterWidget *crtItem);
    void cleanImage(CharacterWidget *crtItem);
    void modifyCharacter(CharacterWidget *crtItem);
    void removeCharacter(CharacterWidget *crtItem);
};
class CharacterPlaceholderWidget : public QWidget
{
     Q_OBJECT
public:
    CharacterPlaceholderWidget(QWidget *parent=nullptr);

signals:
    void addCharacter();

    // QWidget interface
protected:
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
};

class TagPanel : public QWidget
{
    Q_OBJECT
public:
    TagPanel(QWidget *parent = nullptr, bool allowDelete=false, bool checkAble=false, bool allowAdd=false);
    void addTag(const QStringList &tags);
    void removeTag(const QString &tag);
    void setChecked(const QStringList &tags, bool checked = true);
    QStringList tags() const {return tagList.keys();}

    void clear();
    QStringList getSelectedTags();
signals:
    void tagStateChanged(const QString &tag, bool checked);
    void deleteTag(const QString &tag);
    void tagAdded(const QString &tag);
private:
    QMap<QString,QPushButton *> tagList;
    QMenu *tagContextMenu;
    QPushButton *currentTagButton;
    bool allowCheck, adding;
    QLineEdit *tagEdit;
};
class AnimeDetailInfoPage : public QWidget
{
    Q_OBJECT
public:
    AnimeDetailInfoPage(QWidget *parent = nullptr);
    void setAnime(Anime *anime);
    Anime *curAnime() const {return currentAnime;}
    virtual bool eventFilter(QObject *watched, QEvent *event) override;
signals:
    void playFile(const QString &file);
private:
    Anime *currentAnime;
    QLabel *titleLabel, *coverLabel, *viewInfoLabel;
    const char *editAnimeURL = "kikoplay://edit-anime";
    QWidget *coverLabelShadow;
    QTextEdit *descInfo;
    QStringList epNames;
    EpisodesModel *epModel;
    EpItemDelegate *epDelegate;
    QListWidget *characterList;
    TagPanel *tagPanel;
    QStackedLayout *tagContainerSLayout;
    CaptureListModel *captureModel;

    QWidget *setupDescriptionPage();
    QWidget *setupEpisodesPage();
    QWidget *setupCharacterPage();
    QWidget *setupTagPage();
    QWidget *setupCapturePage();

    void refreshCurInfo();
    CharacterWidget *createCharacterWidget(const Character &crt);
    void setCharacters();
    void setTags();

    void updateCharacter(CharacterWidget *crtItem);
    void selectLocalCharacterImage(CharacterWidget *crtItem);
    void pasteCharacterImage(CharacterWidget *crtItem);
    void cleanCharacterImage(CharacterWidget *crtItem);
    void addCharacter();
    void modifyCharacter(CharacterWidget *crtItem);
    void removeCharacter(CharacterWidget *crtItem);
};

#endif // ANIMEDETAILINFOPAGE_H

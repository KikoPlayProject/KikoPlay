#ifndef ANIMEDETAILINFOPAGE_H
#define ANIMEDETAILINFOPAGE_H

#include <QWidget>
#include <QMap>
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
class CharacterWidget : public QWidget
{
     Q_OBJECT
public:
    CharacterWidget(const Character *character, QWidget *parent=nullptr);
    void refreshIcon();
    const Character *crt;
private:
    QLabel *iconLabel;
signals:
    void updateCharacter(CharacterWidget *crtItem);
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
signals:
    void playFile(const QString &file);
private:
    Anime *currentAnime;
    QLabel *titleLabel, *coverLabel, *viewInfoLabel;
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
};

#endif // ANIMEDETAILINFOPAGE_H

#ifndef ANIMEDETAILINFOPAGE_H
#define ANIMEDETAILINFOPAGE_H

#include <QWidget>
#include <QMap>
struct Anime;
class QLabel;
class QTextEdit;
class EpisodesModel;
class DialogTip;
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
    CharacterWidget(Character *character, QWidget *parent=nullptr);
    void refreshIcon();
    Character *crt;
private:
    QLabel *iconLabel;
protected:
    virtual void mousePressEvent(QMouseEvent *event);
signals:
    void updateCharacter(CharacterWidget *crtItem);
public:
    virtual QSize sizeHint() const;
};
class TagPanel : public QWidget
{
    Q_OBJECT
public:
    TagPanel(QWidget *parent = nullptr, bool allowDelete=false, bool checkAble=false, bool allowAdd=false);
    void addTag(const QStringList &tags);
    void removeTag(const QString &tag);


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
    DialogTip *dialogTip;
    QLabel *titleLabel, *coverLabel, *viewInfoLabel, *loadingLabel;
    QTextEdit *descInfo;
    QStringList epNames;
    EpisodesModel *epModel;
    QListWidget *characterList;
    TagPanel *tagPanel;
    QStackedLayout *tagContainerSLayout;
    CaptureListModel *captureModel;

    void showBusyState(bool on);
    QWidget *setupDescriptionPage();
    QWidget *setupEpisodesPage();
    QWidget *setupCharacterPage();
    QWidget *setupTagPage();
    QWidget *setupCapturePage();
};

#endif // ANIMEDETAILINFOPAGE_H

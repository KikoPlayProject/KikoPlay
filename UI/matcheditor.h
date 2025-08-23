#ifndef MATCHEDITOR_H
#define MATCHEDITOR_H

#include "framelessdialog.h"
#include "MediaLibrary/animeinfo.h"

struct PlayListItem;
class QToolButton;
class QLineEdit;
class MatchEditor;
class QComboBox;
class QListWidget;
class ElaCheckBox;

struct MatchEpInfo
{
    EpInfo ep;
    const PlayListItem *item;
    QString displayTitle;
    bool checked{true};
};

class EpItemWidget : public QWidget
{
    Q_OBJECT
public:
    EpItemWidget(QList<MatchEpInfo> &matchList, int index, const QList<EpInfo> &eps);
    int listIndex() const { return _index; }
    void setIndex(int index);
    int getIndex() const;
    bool checked() const { return _matchList[_index].checked; }
    void setChecked(bool on);
    void refreshEps();
signals:
    void setIndexFrom(int index);

public:
    virtual QSize sizeHint() const;

private:
    QList<MatchEpInfo> &_matchList;
    int _index;
    const QList<EpInfo> &_eps;
    QComboBox *epCombo;
    ElaCheckBox *epCheck;

    void addParentItem(QComboBox *combo, const QString& text) const;
    void addChildItem(QComboBox *combo, const QString& text, int epListIndex) const;
};

class MatchEditor : public CFramelessDialog
{
    Q_OBJECT
public:
    MatchEditor(const PlayListItem *item, QList<const PlayListItem *> *batchItems,
                const QString &searchScriptId = "", QWidget *parent = nullptr);

    //QString anime;
    AnimeLite curAnime;
    EpInfo singleEp;
   // QList<EpInfo> epList;
   // QList<bool> epCheckedList;

    QList<MatchEpInfo> matchEpList;

    QList<const PlayListItem *> *batchItems;
    const PlayListItem *curItem;

private:
    QLineEdit *animeEdit, *epEdit, *epIndexEdit;
    QListWidget *epListView;
    QComboBox *epTypeCombo;
    int pageIndex{0};
    QList<EpInfo> curAnimeEpList;

    QAbstractItemModel *animeModel, *epModel;

    QWidget *setupSearchPage(const QString &srcAnime="", const QString &searchScriptId = "");
    QWidget *setupCustomPage(const QString &srcAnime="", const EpInfo &ep=EpInfo());

    void initEpList();
    QString getCommonPart(const QStringList &titles);
    void refreshEpList();

    // CFramelessDialog interface
protected:
    virtual void onAccept();
    virtual void onClose();
};

#endif // MATCHEDITOR_H

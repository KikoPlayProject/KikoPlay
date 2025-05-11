#ifndef LISTWINDOW_H
#define LISTWINDOW_H

#include <QTreeView>
#include <QVBoxLayout>
#include <QAction>
#include <QActionGroup>
#include <QWidgetAction>
#include <QStackedLayout>
#include <QToolButton>
#include <QLineEdit>
#include <QStyledItemDelegate>
#include "Common/notifier.h"
#include "framelessdialog.h"
class QPlainTextEdit;
class QCheckBox;
struct DanmuComment;
class FloatScrollBar;
class QLineEdit;
class QSortFilterProxyModel;
class SubtitleModel;
class QListView;

class ListWindow : public QWidget, public NotifyInterface
{
    Q_OBJECT
public:
    explicit ListWindow(QWidget *parent = nullptr);    
private:
    void initActions();
    inline QModelIndex getPSParentIndex();
    inline QSharedPointer<DanmuComment> getSelectedDanmu();
    void matchPool(const QString &scriptId = "");

    void initListUI();

    QLineEdit *filterEdit;
    QSortFilterProxyModel *currentProxyModel{nullptr};
    QSortFilterProxyModel *playlistProxyModel{nullptr};
    QSortFilterProxyModel *danmulistProxyModel{nullptr};
    QSortFilterProxyModel *sublistProxyModel{nullptr};

    QWidget *titleContainer{nullptr};
    QWidget *infoTip{nullptr};
    const int infoTipHeight = 40;

    QTreeView *playlistView;
    QStackedLayout *contentStackLayout;
    QAction *act_play,*act_addCollection,*act_addItem,*act_addURL,*act_addFolder, *act_addWebDAVCollection,
            *act_cut,*act_paste,*act_moveUp,*act_moveDown,*act_merge,
            *act_remove,*act_clear, *act_removeMatch, *act_removeInvalid,
            *act_sortSelectionAscending,*act_sortSelectionDescending,*act_sortAllAscending,*act_sortAllDescending,
            *act_noLoopOne,*act_noLoopAll,*act_loopOne,*act_loopAll,*act_random,
            *act_browseFile,*act_autoMatch,*act_exportDanmu,*act_addWebDanmuSource, *act_addLocalDanmuSource, *act_updateDanmu,
            *act_sharePoolCode, *act_shareResourceCode, *act_markBgmCollection, *act_updateFolder, *act_PlayOnOtherDevices;
    QMenu *matchSubMenu, *markSubMenu;
    bool actionDisable;
    QActionGroup *loopModeGroup;
    int matchStatus;
    QWidget *initPlaylistPage();

    QTreeView *danmulistView = nullptr;
    QWidget *initDanmulistPage();
    QAction *act_addOnlineDanmu,*act_addLocalDanmu,*act_editPool,*act_editBlock,
            *act_copyDanmuText,*act_copyDanmuColor,*act_copyDanmuSender,
            *act_blockText,*act_blockColor,*act_blockSender,
            *act_jumpToTime, *act_deleteDanmu;

    QListView *sublistView{nullptr};
    SubtitleModel *subModel{nullptr};
    QAction *act_addSub, *act_saveSub, *act_copySubTime, *act_copySubText, *act_subRecognize, *act_subTranslation;
    QWidget *initSublistPage();


protected:
    virtual void resizeEvent(QResizeEvent *event);
    virtual void dragEnterEvent(QDragEnterEvent *event);
    virtual void dropEvent(QDropEvent *event);
public slots:
    void setCurrentList(int listType);
    int currentList() const;
    void sortSelection(bool allItem,bool ascending);
    void playItem(const QModelIndex &index,bool playChild=true);

    virtual void showMessage(const QString &msg, int flag, const QVariant & = QVariant());
    void updatePlaylistActions();
    void updateDanmuActions();
    int updateCurrentPool();
    void infoCancelClicked();
};

class AddUrlDialog : public CFramelessDialog
{
    Q_OBJECT
public:
    AddUrlDialog(QWidget *parent = nullptr);
    QStringList urls;
    QString collectionTitle;
    bool decodeTitle;
private:
    QPlainTextEdit *edit;
    QCheckBox *newCollectionCheck, *decodeTitleCheck;
    QLineEdit *collectionEdit;
    // CFramelessDialog interface
protected:
    virtual void onAccept();
};

class AddWebDAVCollectionDialog : public CFramelessDialog
{
    Q_OBJECT
public:
    AddWebDAVCollectionDialog(QWidget *parent = nullptr);
    QString title;
    QString url;
    QString user;
    QString password;
    QString port;
    QString path;
private:
    QLineEdit *titleEdit, *urlEdit, *portEdit, *pathEdit, *userEdit, *passwordEdit;
    // CFramelessDialog interface
protected:
    virtual void onAccept();
};

#endif // LISTWINDOW_H

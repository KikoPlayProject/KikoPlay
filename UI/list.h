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
#include <QRegExp>
#include <QStyledItemDelegate>
class DanmuComment;
class FilterBox : public QLineEdit
{
    Q_OBJECT
public:
    explicit FilterBox(QWidget *parent = 0);
    inline Qt::CaseSensitivity caseSensitivity() const
    {
        return m_caseSensitivityAction->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive;
    }
    inline QRegExp::PatternSyntax patternSyntax() const
    {
        return static_cast<QRegExp::PatternSyntax>(m_patternGroup->checkedAction()->data().toInt());
    }

signals:
    void filterChanged();
private:
    QAction *m_caseSensitivityAction;
    QActionGroup *m_patternGroup;
};
class ListWindow : public QWidget
{
    Q_OBJECT
public:
    explicit ListWindow(QWidget *parent = nullptr);

private:
    void initActions();
    inline QModelIndex getPSParentIndex();
    inline QSharedPointer<DanmuComment> &getSelectedDanmu();

    QWidget *infoTip;

    QTreeView *playlistView;
    QToolButton *playlistPageButton,*danmulistPageButton;
    QStackedLayout *contentStackLayout;
    QAction *act_play,*act_addCollection,*act_addItem,*act_addFolder,
            *act_cut,*act_paste,*act_moveUp,*act_moveDown,*act_merge,
            *act_remove,*act_clear,
            *act_sortSelectionAscending,*act_sortSelectionDescending,*act_sortAllAscending,*act_sortAllDescending,
            *act_noLoopOne,*act_noLoopAll,*act_loopOne,*act_loopAll,*act_random,
            *act_manuallyAssociate,*act_autoAssociate;
    bool actionDisable;
    QActionGroup *loopModeGroup;
    QWidget *setupPlaylistPage();

    QTreeView *danmulistView;
    QWidget *setupDanmulistPage();
    QAction *act_addOnlineDanmu,*act_addLocalDanmu,*act_editPool,*act_editBlock,*act_exportAll,
            *act_copyDanmuText,*act_copyDanmuColor,*act_copyDanmuSender,
            *act_blockText,*act_blockColor,*act_blockSender,
            *act_jumpToTime, *act_deleteDanmu;

protected:
    virtual void resizeEvent(QResizeEvent *event);
    virtual void dragEnterEvent(QDragEnterEvent *event);
    virtual void dropEvent(QDropEvent *event);
    virtual void enterEvent(QEvent *event);
    virtual void leaveEvent(QEvent *event);
public slots:
    void sortSelection(bool allItem,bool ascending);
    void playItem(const QModelIndex &index,bool playChild=true);

    void showMessage(QString msg,int flag);
    void updatePlaylistActions();
    void updateDanmuActions();

};

#endif // LISTWINDOW_H

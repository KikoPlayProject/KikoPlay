#ifndef DOWNLOADWINDOW_H
#define DOWNLOADWINDOW_H

#include <QWidget>
class Aria2JsonRPC;
class QLabel;
class QPlainTextEdit;
class QTreeView;
struct DownloadTask;
class CTorrentFileModel;
class DownloadWindow : public QWidget
{
    Q_OBJECT
public:
    explicit DownloadWindow(QWidget *parent = nullptr);
    ~DownloadWindow();
private:
    QWidget *setupLeftPanel();
    QWidget *setupGeneralInfoPage();
    QWidget *setupFileInfoPage();
    QWidget *setupGlobalLogPage();

    QTreeView *downloadView;
    QPlainTextEdit *logView;
    QTreeView *fileInfoView;
    CTorrentFileModel *selectedTFModel;
    QLabel *taskTitleLabel,*taskTimeLabel,*taskDirLabel;

    QAction *act_Pause,*act_Start,*act_Remove,*act_addToPlayList,
            *act_PauseAll,*act_StartAll,*act_BrowseFile,
            *act_CopyURI,*act_SaveTorrent;

    Aria2JsonRPC *rpc;
    DownloadTask *currentTask;
    const int refreshInterval=1000;
    const int backgoundRefreshInterval=10000;
    QTimer *refreshTimer;
    QLabel *downSpeedLabel,*upSpeedLabel;

    void initActions();
    void downloadSelectionChanged();
    void setDetailInfo(DownloadTask *task);
signals:
    void playFile(const QString &path);
protected:
    virtual void showEvent(QShowEvent *event);
    virtual void hideEvent(QHideEvent *event);
};

#endif // DOWNLOADWINDOW_H

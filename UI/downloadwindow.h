#ifndef DOWNLOADWINDOW_H
#define DOWNLOADWINDOW_H

#include <QWidget>
#include <QTreeView>
class Aria2JsonRPC;
class QLabel;
class QPlainTextEdit;
class QStackedLayout;
class QButtonGroup;
struct DownloadTask;
class CTorrentFileModel;
class TorrentTreeView : public QTreeView
{
    Q_OBJECT
    Q_PROPERTY(QColor ignoreColor READ getIgnoreColor WRITE setIgnoreColor)
    Q_PROPERTY(QColor normColor READ getNormColor WRITE setNormColor)
public:
    using QTreeView::QTreeView;

    QColor getIgnoreColor() const {return ignoreColor;}
    void setIgnoreColor(const QColor& color)
    {
        ignoreColor =  color;
        emit ignoreColorChanged(ignoreColor);
    }
    QColor getNormColor() const {return normColor;}
    void setNormColor(const QColor& color)
    {
        normColor = color;
        emit normColorChanged(normColor);
    }
signals:
    void ignoreColorChanged(const QColor &color);
    void normColorChanged(const QColor &color);
private:
    QColor ignoreColor, normColor;
};
class DownloadWindow : public QWidget
{
    Q_OBJECT
public:
    explicit DownloadWindow(QWidget *parent = nullptr);
    ~DownloadWindow();
private:
    const int refreshInterval=1000;
    const int backgoundRefreshInterval=10000;

    QTreeView *downloadView;
    QPlainTextEdit *logView;
    TorrentTreeView *fileInfoView;
    CTorrentFileModel *selectedTFModel;
    QLabel *taskTitleLabel,*taskTimeLabel,*taskDirLabel;

    QAction *act_Pause,*act_Start,*act_Remove,*act_addToPlayList,
            *act_PauseAll,*act_StartAll,*act_BrowseFile,
            *act_CopyURI,*act_SaveTorrent;


    Aria2JsonRPC *rpc;
    DownloadTask *currentTask;

    QLabel *downSpeedLabel,*upSpeedLabel;
    QStackedLayout *rightPanelSLayout;
    QButtonGroup *taskTypeButtonGroup;
    QTimer *refreshTimer;

    QWidget *setupLeftPanel();
    QWidget *setupGeneralInfoPage();
    QWidget *setupFileInfoPage();
    QWidget *setupGlobalLogPage();

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

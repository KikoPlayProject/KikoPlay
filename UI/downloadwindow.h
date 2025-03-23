#ifndef DOWNLOADWINDOW_H
#define DOWNLOADWINDOW_H

#include <QWidget>
#include <QTreeView>
#include <QListView>
#include "Common/notifier.h"
class Aria2JsonRPC;
class QLabel;
class QPlainTextEdit;
class QStackedLayout;
class QButtonGroup;
class QSplitter;
struct DownloadTask;
class CTorrentFileModel;
class PeerModel;
class DialogTip;
class TorrentTreeView;
class ResSearchWindow;
class PeerTreeView : public QTreeView
{
    Q_OBJECT
    Q_PROPERTY(QColor barColor READ getBarColor WRITE setBarColor)
    Q_PROPERTY(QColor borderColor READ getBorderColor WRITE setBorderColor)
    Q_PROPERTY(QColor backgroundColor READ getBackgroundColor WRITE setBackgroundColor)
public:
    using QTreeView::QTreeView;

    QColor getBarColor() const {return barColor;}
    void setBarColor(const QColor& color) { barColor =  color; emit barColorChanged(barColor);}
    QColor getBorderColor() const {return borderColor;}
    void setBorderColor(const QColor& color) { borderColor =  color;  emit borderColorChanged(borderColor);}
    QColor getBackgroundColor() const {return backgroundColor;}
    void setBackgroundColor(const QColor& color) { backgroundColor =  color;  emit backgroundColorChanged(backgroundColor);}
signals:
    void borderColorChanged(const QColor &color);
    void barColorChanged(const QColor &color);
    void backgroundColorChanged(const QColor &color);
private:
    QColor borderColor, barColor, backgroundColor;
};

class BlockWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QColor borderColor READ getBorderColor WRITE setBorderColor)
    Q_PROPERTY(QColor fColor READ getFColor WRITE setFColor)
    Q_PROPERTY(QColor uColor READ getUColor WRITE setUColor)
public:
    BlockWidget(QWidget *parent = nullptr);

    QColor getBorderColor() const {return borderColor;}
    void setBorderColor(const QColor& color) { borderColor =  color; }
    QColor getFColor() const {return fillColorF;}
    void setFColor(const QColor& color) { fillColorF =  color; }
    QColor getUColor() const {return fillColorU;}
    void setUColor(const QColor& color) { fillColorU =  color; }

    void setBlock(int count, const QString &state);

    // QWidget interface
protected:
    virtual void paintEvent(QPaintEvent *event);
    virtual void resizeEvent(QResizeEvent *event);
private:
    int blockWidth = 14;
    int blockHeight = 14;
    int marginX = 2;
    int marginY = 2;
    QColor borderColor, fillColorF, fillColorU;
    int blockCount = 0;
    QVector<int> blockState;
};

class DownloadWindow : public QWidget, public NotifyInterface
{
    Q_OBJECT
public:
    explicit DownloadWindow(QWidget *parent = nullptr);
    void beforeClose();
private:
    const int refreshInterval=1000;
    const int backgoundRefreshInterval=10000;

    QSplitter *contentSplitter;
    // QTreeView *downloadView;
    QListView *downloadView;
    TorrentTreeView *fileInfoView;
    BlockWidget *blockView;
    CTorrentFileModel *selectedTFModel;
    QLabel *taskTitleLabel,*taskTimeLabel;

    PeerModel *peerModel;

    QAction *act_Pause,*act_Start,*act_Remove,*act_addToPlayList,
            *act_PauseAll,*act_StartAll,*act_BrowseFile,
            *act_CopyURI,*act_SaveTorrent;


    Aria2JsonRPC *rpc;
    DownloadTask *currentTask;

    QLabel *downSpeedLabel,*upSpeedLabel,*downSpeedIconLabel,*upSpeedIconLabel;
    DialogTip *dialogTip;
    QStackedLayout *rightPanelSLayout;
    QButtonGroup *taskTypeButtonGroup;
    QTimer *refreshTimer;

    ResSearchWindow *resSearchWindow;

    QWidget *initLeftPanel(QWidget *parent);
    QWidget *initDownloadPage();
    QWidget *setupGeneralInfoPage(QWidget *parent);
    QWidget *setupFileInfoPage(QWidget *parent);
    QWidget *setupBlockPage(QWidget *parent);
    QWidget *setupConnectionPage(QWidget *parent);
    QWidget *setupGlobalLogPage(QWidget *parent);

    void initActions();
    void downloadSelectionChanged();
    void setDetailInfo(DownloadTask *task);

    void addUrlTask(const QStringList &urls = QStringList(), const QString &path = "");
    void addTorrentTask();
    void showSettings();
signals:
    void playFile(const QString &path);
    void totalProgressUpdate(int progress);
protected:
    virtual void showEvent(QShowEvent *event);
    virtual void hideEvent(QHideEvent *event);

    // NotifyInterface interface
public:
    virtual void showMessage(const QString &content, int flag, const QVariant & = QVariant());
};

#endif // DOWNLOADWINDOW_H

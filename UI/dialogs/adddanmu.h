#ifndef ADDDANMU_H
#define ADDDANMU_H

#include "UI/framelessdialog.h"
#include "Play/Danmu/common.h"

class QLineEdit;
class QComboBox;
class QListWidget;
class QPlainTextEdit;
struct PlayListItem;
class AddDanmu;
class ElaPivot;
class ScriptSearchOptionPanel;
struct SearchDanmuInfo
{
    DanmuSource src;
    QList<DanmuComment *> danmus;
    bool checked{true};
    QString pool;
};

class SearchItemWidget:public QWidget
{
    Q_OBJECT
public:
    SearchItemWidget(const DanmuSource &item);
    DanmuSource source;
signals:
    void addSearchItem(SearchItemWidget *widgetItem);
public:
    virtual QSize sizeHint() const;
};

class DanmuItemWidget : public QWidget
{
    Q_OBJECT
public:
    DanmuItemWidget(QList<SearchDanmuInfo> &danmuList, int index, const QStringList &poolList);
    int listIndex() const { return _index; }
    void setPoolIndex(int index);
    int getPoolIndex() const;
    bool checked() const { return _danmuList[_index].checked; }
signals:
    void setPoolIndexFrom(int index);

public:
    virtual QSize sizeHint() const;
private:
    QList<SearchDanmuInfo> &_danmuList;
    int _index;
    const QStringList &_danmuPools;
    QComboBox *poolCombo;
};

class RelWordWidget : public QWidget
{
    Q_OBJECT
public:
    explicit RelWordWidget(QWidget *parent=nullptr);
    void setRelWordList(const QStringList &relWords);
    bool isEmpty() const {return relBtns.isEmpty();}
private:
    QList<QPushButton *> relBtns;
signals:
    void relWordClicked(const QString &relWord);
    // QWidget interface
public:
    virtual QSize sizeHint() const;
};

class AddDanmu : public CFramelessDialog
{
    Q_OBJECT
public:
    explicit AddDanmu(const PlayListItem *item, QWidget *parent = nullptr, bool autoPauseVideo=true, const QStringList &poolList=QStringList());

    QList<SearchDanmuInfo> danmuInfoList;

private:
    const PlayListItem *curItem;
    QString defaultPool;
    QLineEdit *keywordEdit;
    ElaPivot *tab{nullptr};
    QPlainTextEdit *urlEdit;
    QPushButton *searchButton, *addUrlButton;
    QComboBox *sourceCombo;
    QListWidget *searchResultWidget;
    QListWidget *selectedDanmuView;
    QString providerId;
    QStringList danmuPools;
    RelWordWidget *relWordWidget;
    QString themeWord;
    ScriptSearchOptionPanel *scriptOptionPanel;

    void search();
    int addSearchItem(QList<DanmuSource> &sources);
    void addURL();
    QWidget *setupSearchPage();
    QWidget *setupURLPage();
    QWidget *setupSelectedPage();
    int processCounter;
    void beginProcrss();
    void endProcess();
    void setPoolIdInSequence(int row, int poolIndex);

protected:
    virtual void onAccept();
    virtual void onClose();

    // QObject interface
public:
    virtual bool eventFilter(QObject *watched, QEvent *event);
};

#endif // ADDDANMU_H

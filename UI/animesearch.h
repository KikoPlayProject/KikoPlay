#ifndef BANGUMISEARCH_H
#define BANGUMISEARCH_H
#include "framelessdialog.h"
#include "MediaLibrary/animeinfo.h"
class QTreeView;
class QLineEdit;
class QComboBox;
class ScriptSearchOptionPanel;

class BgmListModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit BgmListModel(QObject *parent = nullptr) : QAbstractItemModel(parent) {}
    void setAnimes(const QList<AnimeLite> &animes);

public:
    enum struct Columns
    {
        TITLE,
        EXTRA,
        UNKNOWN,
    };

    inline virtual int columnCount(const QModelIndex &) const override { return (int)Columns::UNKNOWN; }
    virtual QModelIndex index(int row, int column, const QModelIndex &parent) const override { return parent.isValid() ? QModelIndex() : createIndex(row,column); }
    virtual QModelIndex parent(const QModelIndex &child) const override { return QModelIndex(); }
    virtual int rowCount(const QModelIndex &parent) const override { return parent.isValid() ? 0 :_animes.size(); }
    virtual QVariant data(const QModelIndex &index, int role) const override;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

private:
    QList<AnimeLite> _animes;
};

class AnimeSearch : public CFramelessDialog
{
    Q_OBJECT
public:
    explicit AnimeSearch(Anime *anime, QWidget *parent = nullptr);
    AnimeLite curSelectedAnime;

private:
    QTreeView *bangumiList;
    BgmListModel *model;
    QLineEdit *searchWordEdit;
    QPushButton *searchButton;
    QComboBox *scriptCombo;
    ScriptSearchOptionPanel *scriptOptionPanel;
    void search();
    // CFramelessDialog interface
protected:
    virtual void onAccept();
};



#endif // BANGUMISEARCH_H

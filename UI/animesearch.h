#ifndef BANGUMISEARCH_H
#define BANGUMISEARCH_H
#include "framelessdialog.h"
#include "MediaLibrary/animeinfo.h"
class QTreeWidget;
class QLineEdit;
class QComboBox;
class AnimeSearch : public CFramelessDialog
{
    Q_OBJECT
public:
    explicit AnimeSearch(Anime *anime, QWidget *parent = nullptr);
    AnimeLite curSelectedAnime;

private:
    QTreeWidget *bangumiList;
    QLineEdit *searchWordEdit;
    QPushButton *searchButton;
    QComboBox *scriptCombo;
    void search();
    // CFramelessDialog interface
protected:
    virtual void onAccept();
};

#endif // BANGUMISEARCH_H

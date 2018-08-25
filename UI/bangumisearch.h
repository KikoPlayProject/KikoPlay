#ifndef BANGUMISEARCH_H
#define BANGUMISEARCH_H
#include "framelessdialog.h"
class QTreeWidget;
class QLineEdit;
struct Anime;
class BangumiSearch : public CFramelessDialog
{
    Q_OBJECT
public:
    explicit BangumiSearch(Anime *anime, QWidget *parent = nullptr);
    Anime *currentAnime;
private:
    QTreeWidget *bangumiList;
    QLineEdit *searchWordEdit;
    QPushButton *searchButton;
    void search();
    // CFramelessDialog interface
protected:
    virtual void onAccept();
};

#endif // BANGUMISEARCH_H

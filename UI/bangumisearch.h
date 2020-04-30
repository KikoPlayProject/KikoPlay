#ifndef BANGUMISEARCH_H
#define BANGUMISEARCH_H
#include "framelessdialog.h"
class QTreeWidget;
class QLineEdit;
class QComboBox;
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
    QLabel *downloadInfoLabel;
    QComboBox *qualityCombo;
    void search();
    // CFramelessDialog interface
protected:
    virtual void onAccept();
};

#endif // BANGUMISEARCH_H

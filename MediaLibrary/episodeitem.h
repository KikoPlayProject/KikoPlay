#ifndef EPISODEITEM_H
#define EPISODEITEM_H
#include <QAbstractItemModel>
#include <QStyledItemDelegate>
#include <QWidget>
#include "animeinfo.h"
class QComboBox;
class QLineEdit;
class EpInfoEditWidget : public QWidget
{
    Q_OBJECT
public:
    EpInfoEditWidget(QWidget *parent=nullptr);
    void setEpInfo(const EpInfo &curEp, const QList<EpInfo> &eps);
    const EpInfo getEp() const;
private:
    const int EpRole = Qt::UserRole + 1;
    QList<EpInfo> epList;
    QComboBox *epTypeCombo;
    QLineEdit *epIndexEdit;
    QComboBox *epNameEdit;
    void addParentItem(QComboBox *combo, const QString& text) const;
    void addChildItem(QComboBox *combo, const QString& text, int epListIndex) const;

    // QWidget interface
protected:
    virtual void paintEvent(QPaintEvent *event);
};

class EpItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

    Anime *curAnime;
    bool autoGetEpInfo;
public:
    EpItemDelegate(QObject *parent = 0):QStyledItemDelegate(parent),curAnime(nullptr), autoGetEpInfo(true){}

    void setAnime(Anime *anime) {curAnime = anime;}

    void setAutoGetEpInfo(bool on) {autoGetEpInfo = on;}

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;

    inline void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &/*index*/) const override
    {
        editor->setGeometry(option.rect);
    }
};

#endif // EPISODEITEM_H
